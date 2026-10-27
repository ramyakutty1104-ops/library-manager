# Requirements and Architecture

## 1. Where the requirements came from

The task page, its source-material page, and its checklist file were read, along with the supplied seed CSV. They say:

- **Goal:** a library management application using classes, encapsulation, collections, and file persistence.
- **Steps to pass:** (1) design `Book`, `Member` and `Loan` classes with clear responsibilities; (2) implement issue, return, search and availability behaviour; (3) save records safely to a local file and reload them on startup; (4) provide the work plus a short explanation behind one public or view-only link.
- **Project requirements:** an offline library manager for fictional records; import the supplied CSV; support adding, listing, searching, updating, borrowing, returning and persisting; use RAII and standard-library containers and document ownership decisions.
- **CLI behaviour:** invalid input is rejected without crashing and without corrupting stored data.
- **Architecture:** keep data, business rules and terminal interaction separate.
- **Edge cases to test:** empty collections, duplicate IDs, missing files, malformed CSV rows, boundary dates.
- **Capstone checklist:** requirements and architecture documentation, automated tests, build instructions, and a short performance/complexity note.

The pages do **not** define command names, the CSV schema beyond the seed file, the date format, the C++ standard, or the storage format. Those are decisions made here (section 4).

## 2. Requirement → where it is satisfied

| Requirement | Where |
|---|---|
| `Book`, `Member`, `Loan` classes with clear responsibilities | `src/Book.*`, `src/Member.*`, `src/Loan.*`; see `docs/class-diagram.md` |
| Encapsulation | Every class has private data, public read access, and setters only where a change is legitimate (e.g. a book's ID can never change) |
| Collections | `std::map` for books, members and loans; `std::vector` for query results and bulk loading; `std::set` during import |
| Issue | `Library::issueBook` (menu 9) |
| Return | `Library::returnBook` (menu 10) |
| Search | `Library::searchBooks` (menu 2) — ID, title, author, category, year; case-insensitive |
| Availability | `Library::isAvailable`, menu 3 shows the full status, menu 1 shows a status column |
| Add / list / update | Menus 4, 1, 5 (books) and 6, 8, 7 (members); loans in menu 11 |
| Import the supplied CSV | Menu 12 (`Storage::readBooksFile` + `Library::importBooks`); the saved `books.csv` uses the same columns |
| Persist safely and reload on startup | `Storage` — auto-save after every change; atomic write; backups; reload at startup |
| RAII and ownership documented | Section 5 below and the header comment of `src/Library.h` |
| Invalid input rejected without crashing or corrupting data | Every answer is read with `std::getline` and validated; a rejected operation changes nothing (e.g. `updateBook` checks all fields before changing any) |
| Three layers separated | Section 3 |
| Edge cases | Section 6 |
| Automated tests | `tests/unit_tests.cpp` (17 groups, 308 checks) and `tests/run_cli_tests.sh` (44 checks) |
| Build instructions | `README.md`, `Makefile` |
| Complexity note | Section 7 |

## 3. Architecture: three layers

```text
 Terminal  (Cli, main)          menus, prompts, printing — no rules
     │ calls
     ▼
 Business rules  (Library, Validation)   uniqueness, limits, due dates — no files, no printing
     │ uses
     ▼
 Data  (Book, Member, Loan, Date, Storage, CsvFormat)   records and files — no rules about borrowing
```

- `Library` can be (and is) unit-tested with no files and no terminal.
- `Storage` can be tested with a temporary folder and no `Library`.
- `Cli` takes `std::istream`/`std::ostream` references, so it is not tied to the real keyboard.
- `Validation` is a small shared rule set used by all layers, so a bad value is refused the same way whether it was typed, passed to `Library`, or read from a file.

## 4. Decisions made where the task was silent

| Topic | Decision | Why |
|---|---|---|
| Storage format | Two CSV files in a data folder: `books.csv` (the **same 8 columns as the seed file**) and `members.csv` (`member_id,name`) | The seed file can be used directly as a data file, and the files open in Excel |
| Loans in the file | A borrowed book's row carries `borrower_id` and `due_date` (exactly like the seed) | Matches the supplied data; no extra file needed |
| Date format | `YYYY-MM-DD`, strictly validated (real calendar dates only; years 1000–9999) | Matches the seed (`2026-09-05`) |
| Loan period / limit | 14 days; at most 3 active loans per member (constants in `Library.h`) | Typical library rules; easy to change |
| Overdue | A loan is overdue only **after** its due date | On the due date itself the member is still on time |
| IDs | Letters, digits, `-`, `_`; up to 12 characters; stored upper-case (`b001` = `B001`) | Predictable, safe in CSV |
| Members in the seed | The seed names borrowers (`M014`, `M022`) but has no member list. A loan whose member is unknown creates a **placeholder member** named "Unknown member", reported to the user, who can fix the name with menu 7 | Keeps the loan instead of discarding data |
| Saving | Automatically after every successful change | The files always match the screen; no "forgot to save" |
| Language standard | C++17 (`std::filesystem`, `std::optional`) | Widely available; g++ 9+ |
| No deleting | Books and members cannot be deleted | Not required; avoids orphaned loans |

## 5. Ownership and RAII

- **No `new` / `delete`, no raw owning pointers anywhere.** All memory is held by standard containers and `std::string`, which release it automatically when they go out of scope (RAII).
- **`Library` owns the records by value**: `std::map<std::string, Book> books_`, `std::map<std::string, Member> members_`, `std::map<std::string, Loan> loansByBook_`. When the `Library` is destroyed everything is freed.
- **`Loan` refers to a book and a member by ID**, not by pointer or reference, so no pointer can dangle when a map grows or an entry is erased.
- **Queries return copies** (`std::optional<Book>`, `std::vector<Book>`), so a caller can never keep a reference into a container that later changes.
- **`Cli` and `Storage` hold references to `Library`/`Storage` objects they do not own.** They are created in `main()` and outlive the `Cli`.
- **Files** are opened with `std::ifstream` / `std::ofstream`, which close themselves when they go out of scope, even if an error occurs.
- **Exceptions** are used only for control flow *inside* `Cli` (end-of-input, too-many-attempts); business and storage code report problems with return values (`OperationResult`, error lists).

## 6. Edge cases and how each is handled

| Edge case | Behaviour | Tested by |
|---|---|---|
| **Empty collections** | Friendly message ("The library has no books yet…", "No books are currently borrowed."); searching an empty library returns nothing | CL-02, unit: Library books |
| **Duplicate IDs** (typed) | Refused with a message, even if letter case differs (`b001` vs `B001`); existing record untouched | CL-18, unit: Library books/members |
| **Duplicate IDs** (in a file) | The first row wins; later duplicates are reported by line number and skipped | unit: Storage malformed rows |
| **Duplicate IDs** (import over existing data) | Existing record kept; duplicate skipped and noted; its loan ignored | CL-05, unit: Library import |
| **Missing files** | Normal on first run: "No saved library found…"; a missing import file gives "file not found" | CL-01, CL-28, unit: Storage missing |
| **Wrong / unreadable file** | A wrong header is refused; a folder where a file is expected is reported | CL-30, CL-32, unit: Storage |
| **Malformed CSV rows** | Wrong column count, bad quotes, bad year, impossible date, bad status, missing borrower, "available" with a borrower — each skipped and reported with its **line number**; good rows still load | CL-29, unit: Storage malformed rows |
| **Damaged saved data** | Reported at startup; before the file is overwritten, the original is copied to `.bak` | CL-31, CL-32 |
| **Boundary dates** | Due date = today + 14 across month end, year end and the leap day (2024-02-29); `2023-02-29`, `2026-02-30`, `2026-13-01`, `2026-04-31` rejected; on the due date → not overdue; the day after → 1 day overdue | CL-10…CL-13, unit: Date, Loan boundaries |
| **Invalid operations** | Unknown book/member; issuing a borrowed book; returning a book that is not out; 4th loan for a member; empty/over-long/control-character text; years outside 1000–2100 | CL-14…CL-20, unit: Library loans |
| **Invalid menu input** | Letters, numbers out of range, empty line, decimals — "Invalid choice"; no hang | CL-24, CL-25 |
| **End of input** | Exits cleanly; a half-entered operation is discarded, nothing is saved | CL-26, CL-27 |
| **Cannot save** (read-only disk, bad path) | Warning shown, data kept in memory, one more attempt at exit, then a clear "could NOT be saved" warning; no crash | CL-33, unit: Storage backup and failures |
| **Excel / Windows files** | UTF-8 BOM and `CRLF` line endings accepted | CL-34, unit: Storage BOM and CRLF |
| **Commas / quotes in text** | Written with standard CSV quoting and read back identically | CL-23, unit: Storage round trip |

## 7. Performance and complexity note

Let *n* = books, *m* = members, *L* = active loans, and *s* = the length of a text field (at most 100 characters).

| Operation | Cost |
|---|---|
| Find / add / update a book or member | O(log n) or O(log m) (ordered `std::map`) |
| Issue a book | O(log n + log m + L) — the member's current loan count is found by scanning the loans |
| Return a book | O(log n + log L) |
| Availability check | O(log n + log L) |
| Search | O(n · s) — one pass over all books, case-insensitive substring match |
| List books / members / loans | O(n) / O(m) / O(L log L) (loans are sorted by due date) |
| Load at startup | O((n + m + L) log n) |
| Save | O(n + m + L log L), and it runs after **every** change |

Memory use is O(n + m + L).

Honest assessment: for the intended size (hundreds to a few thousand records) every action is instant. The weak spots at larger scale would be (a) the full file rewrite after every change (O(n) per change) and (b) the linear search. Both are acceptable here; a much larger system would use an append-only log or a database and an index for search. The member loan-count scan could be made O(1) by keeping a per-member counter, at the price of one more thing to keep consistent.
