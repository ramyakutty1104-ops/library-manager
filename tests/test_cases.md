# Test Cases — Library Manager

## How the tests were run

```bash
make test        # builds ./library_manager and ./unit_tests, runs both suites
```

Or separately, from the project folder:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow src/*.cpp -o library_manager
make unit_tests && ./unit_tests
bash tests/run_cli_tests.sh ./library_manager
```

There are two automated suites:

- **`tests/unit_tests.cpp`** — tests the classes directly (no terminal): `Date`, `Loan`, text helpers, validation, CSV parsing, `Library` rules, and `Storage` (using temporary folders).
- **`tests/run_cli_tests.sh`** — starts the **real program** with scripted keyboard input in a temporary data folder, with `--today 2026-09-08` so dates are predictable, and checks the output, exit code and saved files. A 5-second timeout catches hangs.

## Results (actually run)

| Check | Result |
|---|---|
| Build, `g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow` (g++ 13.3, Linux) | 0 warnings |
| Same flags with `-O2`; and with `-std=c++20` | 0 warnings |
| Unit tests | **17 of 17 groups passed, 308 checks, 0 failed** |
| End-to-end CLI tests | **44 passed, 0 failed** |
| Both suites on a build with AddressSanitizer + UndefinedBehaviorSanitizer | 17/17 groups and 44/44 passed, no sanitizer reports |
| Mutation check (a deliberate bug: "overdue" changed from `>` to `>=` in a scratch copy) | Both suites detected it (1 unit group and CL-10 failed), so the boundary tests do work |

One test *expectation* was wrong on its first run and was corrected (the program itself was right): CL-10
also listed all books, and so legitimately showed a different overdue book (B004). Nothing was changed in the
program to make a test pass.

## End-to-end cases (`tests/run_cli_tests.sh`)

All of these passed.

| ID | Scenario | What is checked |
|---|---|---|
| CL-01 | Start with no saved data | "No saved library found", menu shown, clean exit |
| CL-02 | Empty collections | Friendly messages for list/search/members/loans; unknown book ID rejected, 3 attempts then cancel |
| CL-03 | Import the supplied seed CSV | 6 books, 2 loans, 0 malformed; list shows them; files `books.csv` and `members.csv` created |
| CL-04 | Restart | "Loaded 6 book(s), 2 member(s) and 2 active loan(s)" — data persisted |
| CL-05 | Import the same file twice | 0 added, 6 skipped as duplicates |
| CL-06 | Add member, issue a book, check availability | Due date today + 14; status shows borrower |
| CL-07 / CL-08 | Restart, return on time | Loan survived the restart; return frees the book; the return is also saved |
| CL-09 | Overdue book returned | "OVERDUE by 3 day(s)"; returned "3 day(s) late" |
| CL-10 | **Boundary:** today = due date | Not overdue |
| CL-11 | **Boundary:** one day after due date | "OVERDUE by 1 day(s)" |
| CL-12 | **Boundary:** issue on 2026-12-25 | Due 2027-01-08 (year rollover) |
| CL-13 | **Boundary:** issue on 2024-02-20 | Due 2024-03-05 (leap day) |
| CL-14 | Issue a book that is already borrowed | Refused, names the current borrower |
| CL-15 | 4th book for one member | Refused (limit of 3) |
| CL-16 | Unknown member; return a book that is not out | Both refused with clear messages |
| CL-17 | Add a book with invalid ID, empty title, non-numeric and out-of-range year | Each rejected, then a valid entry works |
| CL-18 | **Duplicate book ID** (also in a different letter case) | Refused; 3 failures cancel; a new ID then works |
| CL-19 | Update a book (Enter keeps other fields) | Only the entered fields change |
| CL-20 | Update with no changes; update with bad then valid year | "No changes entered"; bad year rejected, valid accepted |
| CL-21 | Rename a member; duplicate member ID | Rename works; duplicate refused |
| CL-22 | Search by title, year, author, category; empty text; no match | Case-insensitive matches; empty and no-match handled |
| CL-23 | Title and author containing commas and quotes | Survive save and reload unchanged |
| CL-24 | Seven different invalid menu entries (`abc`, `13`, `-1`, empty, `1.5`, huge number, `7 x`) | All 7 rejected; program exits normally |
| CL-25 | 40 invalid choices in a row | No hang, clean exit |
| CL-26 | End of input at the menu | "End of input reached", exit code 0 |
| CL-27 | End of input in the middle of "Add a book" | Clean exit; the half-entered book is **not** saved |
| CL-28 | Import a file that does not exist | "file not found"; program continues |
| CL-29 | **Malformed CSV rows** in an import | Bad rows skipped and reported with line numbers; good rows imported |
| CL-30 | Import a file with the wrong header | Refused; nothing imported |
| CL-31 | Saved `books.csv` contains a broken row | Warning with line number; good rows load; original kept as `.bak` before overwriting |
| CL-32 | Saved `books.csv` has the wrong header | Not trusted but **not lost** — preserved as `.bak` |
| CL-33 | Data folder cannot be written (path is a file) | Warning; program keeps running; final "could NOT be saved" notice; exit code 0 |
| CL-34 | Windows (`CRLF`) line endings in the input | Handled |
| CL-35 | No `--today` given | Uses the computer's date |
| CL-36…39 | `--help`; unknown option; impossible `--today 2026-02-30`; option without a value | Usage / error message; exit code 0 / 2 / 2 / 2 |

## Unit-test groups (`tests/unit_tests.cpp`)

All 17 passed (308 checks).

| Group | Covers |
|---|---|
| Date | Valid/invalid dates (17 bad examples incl. `2023-02-29`, `2026-02-30`, `2026-13-01`, `2026-04-31`, `1900-02-29`), leap years, adding days across month/year/leap-day boundaries, differences, comparisons |
| Loan boundaries | Not overdue before and **on** the due date; overdue after; days-overdue counts |
| TextUtil | Space normalising, trimming, case, number parsing (rejects `-5`, `12a`, `1.5`, 10 digits…), UTF-8-safe truncation |
| Validation | ID, text and year limits (exact boundaries 12/13, 100/101, 999/1000, 2100/2101) |
| CSV format | Plain, empty, quoted, escaped-quote fields; unterminated and misplaced quotes; write → read round trip |
| Library: books | Add; **duplicate IDs (case-insensitive)**; invalid fields; atomic update (a bad field changes nothing) |
| Library: members | Add, duplicate, rename, invalid |
| Library: loans | Issue, already borrowed, unknown book/member, 3-loan limit, return on time / on the due date / 1 day late / 10 days late, double return |
| Library: search and ordering | Search by each field, case, spaces, empty query, no match; sort orders |
| Library: import | Duplicates skipped, existing records win, placeholder member for an unknown borrower, importing twice changes nothing, invalid members skipped |
| Storage: missing / empty / wrong files | Missing file, empty file, header only, a folder instead of a file, wrong header |
| Storage: supplied seed file | The real `data/library_records_seed.csv`: 6 books, 2 loans, correct borrower and due date |
| Storage: malformed rows | 9 different bad rows, each reported with the right line number; blank line ignored; 2 good rows loaded |
| Storage: BOM and CRLF | Excel-style file loads cleanly |
| Storage: members file | Bad columns, empty name, duplicate ID, quoted comma |
| Storage: save/load round trip | Creates missing folder; no `.tmp` left behind; commas/quotes survive; loan and due date survive; saving twice is identical |
| Storage: backup and failures | Backup created after a damaged load; none for a clean file; saving to an impossible path returns an error instead of crashing |

## Not tested

- Windows and macOS (everything was built and run on Linux only).
- Typing by hand in an interactive terminal. The end-to-end tests feed the same text through a pipe; please also run the program yourself when taking screenshots.
- Non-English (non-ASCII) text in tables: entry and saving work, but column alignment is not guaranteed.
- Two programs using the same data folder at once (not supported).
