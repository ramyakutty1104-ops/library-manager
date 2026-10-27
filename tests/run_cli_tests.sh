#!/usr/bin/env bash
# End-to-end tests: run the real program with scripted input and check what it
# prints and what it saves. Every run uses its own temporary data folder and a
# fixed "today" (2026-09-08) so due-date results are predictable.
#
# Usage (from anywhere, after compiling):
#   bash tests/run_cli_tests.sh                      # uses ./library_manager
#   bash tests/run_cli_tests.sh path/to/binary
#
# Needs bash and `timeout` (Linux, macOS with coreutils, WSL, Git Bash).
# A 5-second timeout catches hangs / endless loops.

cd "$(dirname "$0")/.." || exit 2
BIN="${1:-./library_manager}"
SEED="data/library_records_seed.csv"
pass=0
fail=0
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

if [ ! -x "$BIN" ]; then
    echo "Binary '$BIN' not found. Build it first:  make   (or see README.md)"
    exit 2
fi

newdir() { mktemp -d "$WORK/d.XXXXXX"; }

# run <datadir> <input> [extra program args]   -> sets OUT and CODE
run() {
    local dir="$1" input="$2"
    shift 2
    OUT=$(printf '%b' "$input" | timeout 5 "$BIN" --data-dir "$dir" --today "${TODAY:-2026-09-08}" "$@" 2>&1)
    CODE=$?
}

# check <name> <text that must appear | !text that must NOT appear> ...
# Also requires exit code 0 unless WANT_CODE is set.
check() {
    local name="$1"
    shift
    local ok=1 msgs="" want="${WANT_CODE:-0}"
    if [ "$CODE" -ne "$want" ]; then
        ok=0
        msgs+="    exit code was $CODE, expected $want (124 = timed out / hung)"$'\n'
    fi
    local expected
    for expected in "$@"; do
        if [[ "$expected" == '!'* ]]; then
            if [[ "$OUT" == *"${expected:1}"* ]]; then
                ok=0
                msgs+="    unexpected text present: ${expected:1}"$'\n'
            fi
        elif [[ "$OUT" != *"$expected"* ]]; then
            ok=0
            msgs+="    expected text missing: $expected"$'\n'
        fi
    done
    finish "$name" "$ok" "$msgs"
}

# check_count <name> <text> <expected number of occurrences>
check_count() {
    local name="$1" text="$2" want="$3" ok=1 msgs=""
    local got
    got=$(grep -o -F -- "$text" <<<"$OUT" | wc -l | tr -d ' ')
    if [ "$got" -ne "$want" ]; then
        ok=0
        msgs+="    '$text' appeared $got time(s), expected $want"$'\n'
    fi
    if [ "$CODE" -ne 0 ]; then
        ok=0
        msgs+="    exit code was $CODE"$'\n'
    fi
    finish "$name" "$ok" "$msgs"
}

# check_true <name> <command...>   (passes when the command succeeds)
check_true() {
    local name="$1"
    shift
    if "$@"; then finish "$name" 1 ""; else finish "$name" 0 "    condition failed: $*"$'\n'; fi
}

finish() {
    if [ "$2" -eq 1 ]; then
        echo "PASS  $1"
        pass=$((pass + 1))
    else
        echo "FAIL  $1"
        printf '%s' "$3"
        fail=$((fail + 1))
    fi
}

# ------------------------------------------------------------------ startup --
d=$(newdir)
run "$d" "0\n"
check "CL-01 Empty start: no saved data, menu shown, clean exit" \
    "No saved library found" "===== Library Manager =====" " 1. List all books" \
    "12. Import books from a CSV file" " 0. Exit" "Goodbye!"

run "$d" "1\n2\nx\n8\n11\n3\nB001\nB002\nB003\n0\n"
check "CL-02 Empty collections give friendly messages" \
    "The library has no books yet" "No books match \"x\"" "There are no members yet" \
    "No books are currently borrowed" "No book with ID B001" "Too many invalid attempts"

# ----------------------------------------------------- import and persistence --
d=$(newdir)
run "$d" "12\n$SEED\n1\n0\n"
check "CL-03 Import the supplied seed CSV and list it" \
    "6 book(s) added, 0 skipped, 2 loan(s) added, 0 malformed row(s) ignored" \
    "The Pragmatic Programmer" "Introduction to Algorithms" "Deep Work" \
    "6 book(s), 2 currently borrowed"
check_true "CL-03b Import saved books.csv and members.csv" test -f "$d/books.csv" -a -f "$d/members.csv"

run "$d" "1\n8\n0\n"
check "CL-04 Data is reloaded on the next start" \
    "Loaded 6 book(s), 2 member(s) and 2 active loan(s)" "The Pragmatic Programmer" \
    "M014" "M022"

d=$(newdir)
run "$d" "12\n$SEED\n12\n$SEED\n0\n"
check "CL-05 Importing the same file twice skips duplicates" \
    "0 book(s) added, 6 skipped" "already exists; skipped"

# ------------------------------------------------------ borrow / return flow --
d=$(newdir)
run "$d" "12\n$SEED\n6\nM030\nAlice Johnson\n9\nB001\nM030\n3\nB001\n0\n"
check "CL-06 Add member, issue a book, check availability" \
    "Added member M030 (Alice Johnson)." \
    "Issued \"The Pragmatic Programmer\" (B001) to Alice Johnson (M030). Due 2026-09-22." \
    "Status:   Borrowed by M030 (Alice Johnson), due 2026-09-22"

run "$d" "3\nB001\n11\n10\nB001\n3\nB001\n0\n"
check "CL-07 Loan survives a restart; return on time frees the book" \
    "Borrowed by M030 (Alice Johnson), due 2026-09-22" \
    "Returned \"The Pragmatic Programmer\" (B001). Returned on time." "Status:   Available"

run "$d" "3\nB001\n0\n"
check "CL-08 The return was saved too" "Status:   Available" "!Borrowed by M030"

d=$(newdir)
run "$d" "12\n$SEED\n3\nB002\n10\nB002\n3\nB002\n0\n"
check "CL-09 Overdue book is flagged, then returned late" \
    "Borrowed by M014 (Unknown member), due 2026-09-05 - OVERDUE by 3 day(s)" \
    "It was 3 day(s) late (due 2026-09-05)." "Status:   Available"

d=$(newdir)
TODAY=2026-09-05 run "$d" "12\n$SEED\n3\nB002\n0\n"
check "CL-10 Boundary: on the due date the book is NOT overdue" \
    "Borrowed by M014 (Unknown member), due 2026-09-05" "!OVERDUE"
d=$(newdir)
TODAY=2026-09-06 run "$d" "12\n$SEED\n3\nB002\n0\n"
check "CL-11 Boundary: one day after the due date it is overdue by 1" \
    "OVERDUE by 1 day(s)"
d=$(newdir)
TODAY=2026-12-25 run "$d" "12\n$SEED\n6\nM030\nAlice\n9\nB001\nM030\n0\n"
check "CL-12 Boundary: due date rolls over the year end (2026-12-25 + 14)" \
    "Due 2027-01-08."
d=$(newdir)
TODAY=2024-02-20 run "$d" "12\n$SEED\n6\nM030\nAlice\n9\nB001\nM030\n0\n"
check "CL-13 Boundary: due date handles a leap day (2024-02-20 + 14)" \
    "Due 2024-03-05."

d=$(newdir)
run "$d" "12\n$SEED\n6\nM030\nAlice\n9\nB002\nM030\n0\n"
check "CL-14 Cannot issue a book that is already borrowed" \
    "Error: Book B002 is already borrowed by M014 (due 2026-09-05)."

d=$(newdir)
run "$d" "12\n$SEED\n6\nM030\nAlice\n9\nB001\nM030\n9\nB003\nM030\n9\nB005\nM030\n9\nB006\nM030\n8\n0\n"
check "CL-15 A member may hold at most 3 books" \
    "Error: Member M030 already has 3 active loans, which is the limit."

d=$(newdir)
run "$d" "12\n$SEED\n9\nB001\nM404\nM405\nM406\n10\nB001\n0\n"
check "CL-16 Unknown member rejected; returning a book that is not out is refused" \
    "No member with ID M404" "Too many invalid attempts" \
    "Error: Book B001 is not currently borrowed."

# ------------------------------------------------------ add / update / search --
d=$(newdir)
run "$d" "4\nB 7\nB200\n\nMy Book\nAuthor\nCat\nabc\n3000\n2001\n0\n"
check "CL-17 Add a book: invalid ID, empty title, bad years are rejected, then it works" \
    "may only contain letters, digits" "Title cannot be empty" \
    "Year must be a whole number" "Year must be between 1000 and 2100" "Added book B200"

run "$d" "4\nb200\nB200\nB200\n4\nB200\nB201\nOther\nA\nC\n1999\n1\n0\n"
check "CL-18 Duplicate book ID is refused (any letter case); retry with a new ID works" \
    "A book with ID B200 already exists." "Too many invalid attempts" "Added book B201" \
    "My Book" "Other"

run "$d" "5\nB200\nNew Title\n\n\n2005\n3\nB200\n0\n"
check "CL-19 Update a book (Enter keeps the other fields)" \
    "Updated book B200." "Title:    New Title" "Author:   Author" "Year:     2005"

run "$d" "5\nB200\n\n\n\n\n5\nB200\n\n\n\nabc\n1800\n0\n"
check "CL-20 Update with no changes; update with bad then valid year" \
    "No changes entered; the book was not modified." "Year must be a whole number" \
    "Updated book B200."

d=$(newdir)
run "$d" "6\nM030\nAlice\n7\nM030\nAlice Cooper\n8\n6\nm030\nM031\nBob\n0\n"
check "CL-21 Rename a member; duplicate member ID refused" \
    "Member M030 is now named Alice Cooper." "Alice Cooper" \
    "A member with ID M030 already exists." "Added member M031 (Bob)."

d=$(newdir)
run "$d" "12\n$SEED\n2\nclean\n2\nzzz\n2\n\n2\n2008\n2\nMARTIN\n2\nsoftware\n0\n"
check "CL-22 Search by title, year, author, category (case-insensitive); empty and no-match" \
    "1 book(s) found:" "Clean Code" "No books match \"zzz\"." "Search text cannot be empty." \
    "2 book(s) found:"

d=$(newdir)
run "$d" '4\nB400\nTitle, with "quotes"\nAuthor, Jr.\nCat\n2012\n0\n'
run "$d" "3\nB400\n0\n"
check "CL-23 Commas and quotes in text survive saving and loading" \
    'Title:    Title, with "quotes"' "Author:   Author, Jr."

# ------------------------------------------------------------ invalid input --
d=$(newdir)
run "$d" "abc\n13\n-1\n\n1.5\n99999999999\n 7 x\n0\n"
check_count "CL-24 Seven invalid menu choices are all rejected" \
    "Invalid choice. Please enter a number from 0 to 12." 7
check "CL-24b ...and the program still exits normally" "Goodbye!"

d=$(newdir)
run "$d" "$(printf 'x\\n%.0s' {1..40})0\n"
check "CL-25 Forty invalid choices in a row: no hang, clean exit" "Invalid choice" "Goodbye!"

# ------------------------------------------------------- end of input / EOF --
d=$(newdir)
run "$d" ""
check "CL-26 End of input at the menu exits cleanly" "End of input reached. Exiting." "Goodbye!"

d=$(newdir)
run "$d" "4\nB300\nTitle\n"
check "CL-27 End of input in the middle of Add a Book exits cleanly" \
    "End of input reached. Exiting." "Goodbye!"
run "$d" "0\n"
check "CL-27b ...and the half-entered book was not saved" "No saved library found"

# --------------------------------------------------------- bad / missing files --
d=$(newdir)
run "$d" "12\nnope/missing.csv\n0\n"
check "CL-28 Importing a file that does not exist reports it and continues" \
    "Error: file not found: nope/missing.csv" "Goodbye!"

d=$(newdir)
printf 'book_id,title,author,category,year,status,borrower_id,due_date\nB500,Good,Auth,Cat,2000,available,,\nB501,Bad date,Auth,Cat,2000,borrowed,M001,2026-02-30\nB502,Short\nB503,Also good,Auth,Cat,1999,borrowed,M001,2026-10-01\n' > "$WORK/bad_import.csv"
run "$d" "12\n$WORK/bad_import.csv\n1\n0\n"
check "CL-29 Malformed rows are skipped and reported with line numbers" \
    "Warning: 2 problem(s) found" "line 3: due_date must be a real calendar date" \
    "line 4: expected 8 columns but found 2" \
    "2 book(s) added, 0 skipped, 1 loan(s) added, 2 malformed row(s) ignored" \
    "Good" "Also good" "!Bad date  "

d=$(newdir)
printf 'foo,bar\n1,2\n' > "$WORK/wrong_header.csv"
run "$d" "12\n$WORK/wrong_header.csv\n0\n"
check "CL-30 A file with the wrong header is refused" \
    "unexpected header row" "Nothing was imported."

d=$(newdir)
printf 'book_id,title,author,category,year,status,borrower_id,due_date\nB001,Good,Auth,Cat,2000,available,,\nTHIS ROW IS BROKEN\n' > "$d/books.csv"
cp "$d/books.csv" "$WORK/original_books.csv"
run "$d" "6\nM001\nAlice\n1\n0\n"
check "CL-31 Damaged saved file: warning at startup, good rows still load" \
    "Warning: 1 problem(s) found in books.csv" "line 3:" "Loaded 1 book(s)" "Good"
check_true "CL-31b A backup (.bak) of the damaged original was made before overwriting" \
    cmp -s "$d/books.csv.bak" "$WORK/original_books.csv"

d=$(newdir)
printf 'foo,bar\nthese,are,my,notes\n' > "$d/books.csv"
cp "$d/books.csv" "$WORK/original_notes.csv"
run "$d" "6\nM001\nAlice\n0\n"
check "CL-32 A file with the wrong header is not trusted but NOT lost" \
    "unexpected header row" "Added member M001"
check_true "CL-32b ...it is preserved as books.csv.bak" cmp -s "$d/books.csv.bak" "$WORK/original_notes.csv"

d=$(newdir)
printf 'not a folder' > "$WORK/afile"
run "$WORK/afile" "6\nM001\nAlice\n8\n0\n"
check "CL-33 Unwritable data location: warns, keeps running, keeps data in memory" \
    "Warning: could not save your changes" "Added member M001" "M001  Alice" \
    "WARNING: your latest changes could NOT be saved." "Goodbye!"

# -------------------------------------------------------------- misc / args --
d=$(newdir)
run "$d" "12\r\n$SEED\r\n3\r\nB001\r\n0\r\n"
check "CL-34 Windows (CRLF) line endings are handled" \
    "Title:    The Pragmatic Programmer" "Goodbye!"

d=$(newdir)
OUT=$(printf '0\n' | timeout 5 "$BIN" --data-dir "$d" 2>&1)
CODE=$?
check "CL-35 Without --today the computer's date is used" "Today's date: 20" "Goodbye!"

OUT=$("$BIN" --help 2>&1); CODE=$?
check "CL-36 --help prints usage and exits 0" "Usage: library_manager"
OUT=$("$BIN" --bogus 2>&1); CODE=$?
WANT_CODE=2 check "CL-37 Unknown option is rejected (exit code 2)" "unknown option '--bogus'"
OUT=$("$BIN" --today 2026-02-30 2>&1); CODE=$?
WANT_CODE=2 check "CL-38 Impossible --today date is rejected (exit code 2)" "real date written as YYYY-MM-DD"
OUT=$("$BIN" --data-dir 2>&1); CODE=$?
WANT_CODE=2 check "CL-39 Option without a value is rejected (exit code 2)" "needs a value"

echo
echo "Result: $pass passed, $fail failed"
[ "$fail" -eq 0 ]
