# Object-Oriented Library Manager (C++17)

A menu-driven library management program for the terminal. It models a library with
**`Book`**, **`Member`** and **`Loan`** classes, lets you add, list, search, update, issue
and return books, and **saves everything to local CSV files** that are reloaded the next
time the program starts.

It is written in standard C++17 only (no third-party libraries) and is organised in three
separate layers — **data**, **business rules** and **terminal** — so each part is small and
easy to understand and test.

> **About the requirements:** the task page, its source-material page and its checklist were
> read and followed (summary in [`docs/requirements-and-architecture.md`](docs/requirements-and-architecture.md)).
> Where they did not specify something (command names, file layout, loan rules) the choices
> made are listed there under *Decisions made where the task was silent*.

## Features

| Menu | What it does |
|---|---|
| **1. List all books** | Table of ID, title, author, category, year and status (Available / Borrowed / Overdue) |
| **2. Search books** | Case-insensitive search of ID, title, author, category and year |
| **3. Check book availability** | Full details of one book, and who has it and when it is due |
| **4. Add a book** | Prompts for ID, title, author, category, year |
| **5. Update a book** | Change title, author, category or year (Enter keeps the current value) |
| **6. Add a member** | ID and name |
| **7. Update a member's name** | Correct a member's name |
| **8. List members** | Members and how many books each has out |
| **9. Issue (borrow) a book** | Lends a book for 14 days; max 3 books per member |
| **10. Return a book** | Returns it and says if it was late |
| **11. List active loans** | Who has what, due dates, overdue days |
| **12. Import books from a CSV file** | Loads a file such as the supplied seed (`data/library_records_seed.csv`) |
| **0. Exit** | Quit |

Reliability features: every input is validated; wrong input is rejected with a clear message
and never crashes the program or corrupts saved data; each question allows 3 attempts and then
returns to the menu; end-of-input (Ctrl+D / Ctrl+Z) exits cleanly; changes are saved after every
operation; bad rows in a data file are skipped and reported by line number.

## Prerequisites

- A C++17 compiler. Built and tested with **g++ 13.3 on Linux**. Any g++ **9 or newer** should work
  (it uses `std::filesystem`). On Windows use MinGW-w64 / MSYS2 g++, or WSL.
- Optional: `make` (a plain `g++` command is also given), and `bash` to run the end-to-end tests.

```bash
g++ --version
```

## Build

From the project folder (`library-manager/`):

**With make**

```bash
make
```

**With g++ directly** (Linux / macOS / Git Bash / WSL)

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow src/*.cpp -o library_manager
```

**Windows Command Prompt / PowerShell** (if the `*` is not expanded)

```bat
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow src/Book.cpp src/Cli.cpp src/CsvFormat.cpp src/Date.cpp src/Library.cpp src/Loan.cpp src/main.cpp src/Member.cpp src/Storage.cpp src/TextUtil.cpp src/Validation.cpp -o library_manager
```

The build produces no warnings.

## Run

```bash
./library_manager                 # Windows: library_manager.exe
```

Options:

| Option | Meaning |
|---|---|
| `--data-dir <folder>` | Where `books.csv` and `members.csv` are kept (default `library_data`, created automatically) |
| `--today YYYY-MM-DD` | Pretend today is this date — useful for trying out due dates and overdue books |
| `--help` | Show usage |

### First run: load the sample data

1. Start the program. It says no saved library was found.
2. Choose **12** and enter `data/library_records_seed.csv`.
3. Choose **1** to see the six sample books. From now on your data is saved in `library_data/`.

To see overdue handling, start with a date after the sample due dates:

```bash
./library_manager --today 2026-09-08
```

## Sample session

(Text after a prompt is what the user types. Output below is real program output.)

```text
Welcome to the Library Manager!
Today's date: 2026-09-08

Enter your choice (0-12): 12
Path to the CSV file: data/library_records_seed.csv
Note: Member M014 has a loan but was not in the member list; added a placeholder (use 'Update member name' to correct it).
Note: Member M022 has a loan but was not in the member list; added a placeholder (use 'Update member name' to correct it).
Import finished: 6 book(s) added, 0 skipped, 2 loan(s) added, 0 malformed row(s) ignored.

Enter your choice (0-12): 1

--- All Books ---
ID    Title                          Author                        Category          Year  Status
----  -----------------------------  ----------------------------  ----------------  ----  ---------
B001  The Pragmatic Programmer       Andrew Hunt and David Thomas  Software          1999  Available
B002  Clean Code                     Robert C Martin               Software          2008  Overdue
B003  Introduction to Algorithms     Thomas H Cormen et al         Computer Science  2022  Available
B004  The Design of Everyday Things  Don Norman                    Design            2013  Overdue
B005  Deep Work                      Cal Newport                   Productivity      2016  Available
B006  Computer Networks              Andrew S Tanenbaum and Da...  Networking        2021  Available
6 book(s), 2 currently borrowed.

Enter your choice (0-12): 9

--- Issue (Borrow) a Book ---
Book ID: B001
Member ID: M030
Issued "The Pragmatic Programmer" (B001) to Alice Johnson (M030). Due 2026-09-22.

Enter your choice (0-12): 11

--- Active Loans ---
Book  Title                          Member               Due         Status
----  -----------------------------  -------------------  ----------  --------------------
B004  The Design of Everyday Things  M022 Unknown member  2026-08-29  OVERDUE by 10 day(s)
B002  Clean Code                     M014 Unknown member  2026-09-05  OVERDUE by 3 day(s)
B001  The Pragmatic Programmer       M030 Alice Johnson   2026-09-22  On time

Enter your choice (0-12): 10

--- Return a Book ---
Book ID: B002
Returned "Clean Code" (B002). It was 3 day(s) late (due 2026-09-05).
```

(Member `M030` was added earlier with menu option 6.)

Handling bad input:

```text
Enter your choice (0-12): abc
Invalid choice. Please enter a number from 0 to 12.

Year published: abc
  Error: Year must be a whole number such as 2008.
Year published: 3000
  Error: Year must be between 1000 and 2100.

Book ID: B999
  Error: No book with ID B999. Choose option 1 to see all book IDs.
```

## Rules the program enforces

- **IDs:** letters, digits, `-` and `_`, up to 12 characters, no spaces. Stored in upper case, so `b001` means `B001`. IDs must be unique.
- **Text** (title, author, category, name): 1–100 characters, extra spaces tidied, no control characters. Commas and quotes are allowed.
- **Year:** whole number from 1000 to 2100.
- **Issuing:** the book and the member must exist; the book must not already be borrowed; a member may hold at most **3** books. The due date is **14 days** after today.
- **Overdue:** a loan is late only *after* its due date (on the due date itself it is still on time).
- **Dates** in files must be real calendar dates written `YYYY-MM-DD` (so `2023-02-29` and `2026-04-31` are rejected; `2024-02-29` is fine).

## Data files

Saved in the data folder (`library_data/` by default):

`books.csv` — the same columns as the supplied seed file:

```csv
book_id,title,author,category,year,status,borrower_id,due_date
B001,The Pragmatic Programmer,Andrew Hunt and David Thomas,Software,1999,borrowed,M030,2026-09-22
B002,Clean Code,Robert C Martin,Software,2008,available,,
```

`members.csv`:

```csv
member_id,name
M030,Alice Johnson
```

How the files are kept safe:

- **Atomic saving:** each file is first written to `<name>.tmp` and then moved into place, so a crash or full disk in the middle of a save cannot leave a half-written file.
- **Skip, report, never crash:** a malformed row (wrong number of columns, bad quote, bad year, impossible date, unknown status, duplicate ID…) is skipped and reported with its line number; the good rows still load.
- **Backups:** if a data file had problems when it was loaded, the original is copied to `<name>.bak` *before* it is overwritten, so nothing is silently lost.
- **Missing files** are normal on the first run.
- **Excel-friendly:** a UTF-8 BOM and Windows (`CRLF`) line endings are accepted.
- The data files are plain text — you can open them in a spreadsheet or text editor. Edit them only while the program is not running.

## Project structure

```text
library-manager/
├── src/
│   ├── main.cpp          entry point; wires the layers together
│   ├── Cli.h / .cpp      TERMINAL layer: menus, prompts, printing
│   ├── Library.h / .cpp  BUSINESS layer: owns the records, enforces the rules
│   ├── Validation.h/.cpp   shared rules for IDs, text, years
│   ├── Book.h / .cpp     DATA: catalogue record
│   ├── Member.h / .cpp   DATA: borrower
│   ├── Loan.h / .cpp     DATA: "member has book until date"
│   ├── Date.h / .cpp     DATA: strict calendar dates, day arithmetic
│   ├── Storage.h / .cpp  DATA: reads/writes the CSV files safely
│   ├── CsvFormat.h/.cpp  DATA: one line of CSV (quotes, commas)
│   └── TextUtil.h / .cpp tiny text helpers
├── data/
│   └── library_records_seed.csv   the supplied sample records
├── docs/
│   ├── class-diagram.md                  class diagram (Mermaid + text)
│   └── requirements-and-architecture.md  requirements, layers, edge cases, complexity
├── tests/
│   ├── unit_tests.cpp      unit tests (no framework needed)
│   ├── run_cli_tests.sh    end-to-end tests of the real program
│   └── test_cases.md       the test cases and recorded results
├── screenshots/
│   └── README.md           which terminal screenshots to capture
├── Makefile
├── .gitignore
└── README.md
```

See [`docs/class-diagram.md`](docs/class-diagram.md) for the class diagram and
[`docs/requirements-and-architecture.md`](docs/requirements-and-architecture.md) for ownership/RAII
notes, the edge-case table and the performance/complexity note.

## Tests

```bash
make test                          # builds everything, runs both suites
```

or separately:

```bash
make unit_tests && ./unit_tests    # unit tests: dates, CSV, validation, Library rules, Storage
bash tests/run_cli_tests.sh        # end-to-end: runs the real program with scripted input
```

Both must be run from the project folder. Results are recorded in [`tests/test_cases.md`](tests/test_cases.md).

## Design decisions (short version)

- **One source of truth for "borrowed".** A `Book` stores only catalogue facts. Whether it is out is represented *only* by a `Loan` in a map keyed by book ID — so a book can never be both "available" and "borrowed", and can never have two loans. The CSV `status` column is derived from loans when saving.
- **Loans refer to IDs, not pointers.** Nothing can dangle; there is no `new`/`delete` anywhere (RAII with standard containers).
- **Rules never print and never touch files.** `Library` returns an `OperationResult` (ok + message), which makes it easy to test and lets the terminal layer decide how to show it.
- **Validate everything, change nothing on failure.** For example, `updateBook` checks every field before changing any.
- **Strict dates.** A `Date` is stored as a day count since 1970, which makes "+14 days", "days late" and leap years exact.
- **Seed compatibility.** The saved `books.csv` has exactly the seed's columns. The seed names borrowers (`M014`, `M022`) but has no member list, so such members are created as "Unknown member" placeholders that you can rename with menu 7.

## Assumptions

- The CSV layout and `YYYY-MM-DD` dates follow the supplied seed file.
- A single person uses the program at a time (no file locking).
- A 14-day loan period and a 3-book limit are reasonable defaults; they are the constants `kLoanPeriodDays` and `kMaxLoansPerMember` in `src/Library.h`.

## Limitations

- **No history:** when a book is returned the loan is forgotten; only active loans are stored.
- **No delete:** books and members cannot be removed (not required, and it avoids orphaned loans).
- **No fines, renewals or reservations.**
- **Single user:** two copies of the program using the same data folder would overwrite each other.
- **Whole-file saves:** every change rewrites both files (fast for thousands of records, not for millions).
- **Wide tables:** the book list is about 100 characters wide; use a wide terminal. Long text in tables is shortened with `...` (menu 3 shows everything).
- **Text is treated as bytes:** non-English characters can be stored, but the length limits and table alignment count bytes, so columns may look uneven.
- **Platforms:** built and tested on Linux with g++ 13.3 only. It uses only standard C++17, but it has not been run on Windows or macOS.
