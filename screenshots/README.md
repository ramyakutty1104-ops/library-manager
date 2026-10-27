# Screenshots

This folder is for **real terminal screenshots** that you capture yourself by running the
program. None are included on purpose: they must come from your own run and must not be
edited or faked.

## Before you start

Use a **wide terminal window** (at least about 110 columns) so the book table does not wrap.

```bash
cd library-manager
make                                   # or the g++ command from README.md
./library_manager --data-dir demo_data --today 2026-09-08     # Windows: library_manager.exe ...
```

`--data-dir demo_data` keeps your screenshots separate from real data; `--today 2026-09-08`
makes two of the sample books show as **overdue**. Delete the `demo_data` folder afterwards
if you want a fresh start.

## How to capture a screenshot

- **Windows:** `Win + Shift + S`, drag over the terminal, then save the image as PNG.
- **macOS:** `Cmd + Shift + 4`, drag over the terminal (saved to the Desktop).
- **Linux:** your screenshot tool (`PrtSc`, GNOME Screenshot, Flameshot…), save as PNG.

Keep the prompts and the program's replies readable, and do not crop them away.

## Screenshots to capture

Save each in this folder with the suggested name.

| File name | What to do | What it shows |
|---|---|---|
| `01-build-and-menu.png` | Run the build command (no warnings), then start the program | Clean build; welcome text; the full menu |
| `02-import-seed.png` | Choose **12**, enter `data/library_records_seed.csv` | Importing the supplied CSV: "6 book(s) added, 2 loan(s) added" |
| `03-list-books.png` | Choose **1** | Table of books with Available / Overdue status |
| `04-search.png` | Choose **2**, search `martin`, then `zzz`, then press Enter on an empty search | Search results, "no match" message, empty input rejected |
| `05-availability.png` | Choose **3**, enter `B002` | Book details, borrower, due date, "OVERDUE by 3 day(s)" |
| `06-add-book-validation.png` | Choose **4**; enter an ID with a space, an empty title, a non-numeric year, year `3000`, then valid values | Invalid input rejected with clear messages, then success |
| `07-duplicate-id.png` | Choose **4** and enter an existing ID such as `B001` (and `b001`) | Duplicate IDs refused |
| `08-add-member-and-issue.png` | Choose **6** (e.g. `M030`, `Alice Johnson`), then **9** (`B001`, `M030`) | Member added; book issued with a due date 14 days away |
| `09-issue-errors.png` | Choose **9** with `B002` (already borrowed); issue 3 books to `M030` and then a 4th | "Already borrowed" and "limit of 3" refusals |
| `10-return.png` | Choose **10**, return `B002`; then **11** to list loans | Late return message; updated loan list |
| `11-update.png` | Choose **5**, update a book (press Enter to keep fields); choose **7** to rename a member | Update flow and "keep current value" |
| `12-persistence.png` | Choose **0** to exit, start the program again with the same `--data-dir`, then choose **11** | "Loaded 6 book(s)…" — the data was saved and reloaded; show `books.csv` too (e.g. `cat demo_data/books.csv`) |
| `13-invalid-menu.png` | Enter `abc`, `99`, an empty line, `-1` at the menu | Invalid choices rejected; program keeps running |
| `14-bad-csv.png` | Make a copy of the seed, break one row (e.g. change a date to `2026-02-30`), import it with **12** | Malformed row skipped and reported with its line number |
| `15-tests.png` | Run `make test` (or both test commands) | Unit tests and end-to-end tests all passing |

Optional: `16-end-of-input.png` — start the program and press `Ctrl + D` (Linux/macOS) or
`Ctrl + Z` then Enter (Windows) at the menu to show a clean exit.

## Checklist

- [ ] Screenshots come from your own genuine run
- [ ] Prompts and replies are readable (wide terminal, not cropped)
- [ ] Files are saved in this `screenshots/` folder
- [ ] Referenced from your submission page if the task asks for them
