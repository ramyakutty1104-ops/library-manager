#include "Cli.h"

#include <algorithm>
#include <istream>
#include <ostream>

#include "TextUtil.h"
#include "Validation.h"

namespace {

constexpr int kMaxAttempts = 3;
constexpr std::size_t kMaxProblemsShown = 10;
// Longest text shown in a table cell (longer text ends in "..."; option 3 shows everything).
constexpr std::size_t kTitleWidth = 30;
constexpr std::size_t kAuthorWidth = 28;
constexpr std::size_t kCategoryWidth = 16;
constexpr std::size_t kNameWidth = 30;

// Prints rows of text as an aligned table (the last column is not padded).
void printTable(std::ostream& out, const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows) {
    std::vector<std::size_t> widths;
    for (const std::string& header : headers) {
        widths.push_back(header.size());
    }
    for (const std::vector<std::string>& row : rows) {
        for (std::size_t c = 0; c < row.size(); ++c) {
            widths[c] = std::max(widths[c], row[c].size());
        }
    }
    auto printRow = [&](const std::vector<std::string>& cells) {
        for (std::size_t c = 0; c < cells.size(); ++c) {
            if (c + 1 == cells.size()) {
                out << cells[c];
            } else {
                out << textutil::padRight(cells[c], widths[c]) << "  ";
            }
        }
        out << '\n';
    };
    printRow(headers);
    std::vector<std::string> dashes;
    for (std::size_t width : widths) {
        dashes.emplace_back(width, '-');
    }
    printRow(dashes);
    for (const std::vector<std::string>& row : rows) {
        printRow(row);
    }
}

// Removes the quotes people get when they drag a file into a terminal.
std::string stripSurroundingQuotes(const std::string& text) {
    if (text.size() >= 2 && text.front() == '"' && text.back() == '"') {
        return text.substr(1, text.size() - 2);
    }
    return text;
}

}  // namespace

Cli::Cli(Library& library, Storage& storage, const Date& today, std::istream& in,
         std::ostream& out)
    : library_(library), storage_(storage), today_(today), in_(in), out_(out) {}

// ------------------------------------------------------------ main loop ----

void Cli::run() {
    out_ << "Welcome to the Library Manager!\n"
         << "Today's date: " << today_.toString() << '\n';

    loadSavedData();

    bool running = true;
    while (running) {
        try {
            showMenu();
            running = handleChoice(readLine("Enter your choice (0-12): "));
        } catch (const InputCancelled&) {
            out_ << "Too many invalid attempts. The operation was cancelled; "
                    "returning to the menu.\n";
        } catch (const InputEnded&) {
            out_ << "End of input reached. Exiting.\n";
            running = false;
        }
    }

    if (unsavedChanges_) {
        persist();  // one last try
        if (unsavedChanges_) {
            out_ << "WARNING: your latest changes could NOT be saved.\n";
        }
    }
    out_ << "Goodbye!\n";
}

void Cli::showMenu() {
    out_ << "\n===== Library Manager =====\n"
         << " 1. List all books\n"
         << " 2. Search books\n"
         << " 3. Check book availability\n"
         << " 4. Add a book\n"
         << " 5. Update a book\n"
         << " 6. Add a member\n"
         << " 7. Update a member's name\n"
         << " 8. List members\n"
         << " 9. Issue (borrow) a book\n"
         << "10. Return a book\n"
         << "11. List active loans\n"
         << "12. Import books from a CSV file\n"
         << " 0. Exit\n";
}

bool Cli::handleChoice(const std::string& line) {
    int choice = -1;
    if (!textutil::parseInteger(textutil::normalizeSpaces(line), choice)) {
        choice = -1;
    }
    switch (choice) {
        case 0: return false;
        case 1: listBooks(); break;
        case 2: searchBooks(); break;
        case 3: checkAvailability(); break;
        case 4: addBook(); break;
        case 5: updateBook(); break;
        case 6: addMember(); break;
        case 7: renameMember(); break;
        case 8: listMembers(); break;
        case 9: issueBook(); break;
        case 10: returnBook(); break;
        case 11: listLoans(); break;
        case 12: importCsv(); break;
        default: out_ << "Invalid choice. Please enter a number from 0 to 12.\n"; break;
    }
    return true;
}

// ------------------------------------------------- startup and saving ----

void Cli::reportReadProblems(const std::string& fileName, const std::vector<RowError>& errors) {
    if (errors.empty()) {
        return;
    }
    out_ << "Warning: " << errors.size() << " problem(s) found in " << fileName
         << " (those rows were skipped):\n";
    for (std::size_t i = 0; i < errors.size() && i < kMaxProblemsShown; ++i) {
        if (errors[i].line == 0) {
            out_ << "  " << errors[i].message << '\n';
        } else {
            out_ << "  line " << errors[i].line << ": " << errors[i].message << '\n';
        }
    }
    if (errors.size() > kMaxProblemsShown) {
        out_ << "  ... and " << (errors.size() - kMaxProblemsShown) << " more.\n";
    }
}

void Cli::loadSavedData() {
    const MembersFile membersFile = storage_.loadMembers();
    const BooksFile booksFile = storage_.loadBooks();

    if (!membersFile.found && !booksFile.found) {
        out_ << "No saved library found in '" << storage_.dataDir().string()
             << "'. Starting with an empty library.\n"
             << "Tip: choose option 12 and enter data/library_records_seed.csv to load the "
                "sample records.\n";
        return;
    }

    reportReadProblems("members.csv", membersFile.errors);
    reportReadProblems("books.csv", booksFile.errors);
    if (!membersFile.errors.empty() || !booksFile.errors.empty()) {
        out_ << "The original file(s) will be copied to a .bak backup before anything is "
                "overwritten.\n";
    }

    library_.importMembers(membersFile.members);
    const ImportSummary summary = library_.importBooks(booksFile.books, booksFile.loans);
    for (const std::string& note : summary.notes) {
        out_ << "Note: " << note << '\n';
    }
    out_ << "Loaded " << library_.bookCount() << " book(s), " << library_.memberCount()
         << " member(s) and " << library_.activeLoanCount() << " active loan(s) from '"
         << storage_.dataDir().string() << "'.\n";
}

// Saves everything. Called after every successful change, so the files on disk
// always match what is on screen.
void Cli::persist() {
    std::string error;
    if (storage_.save(library_.books(), library_.members(), library_.loans(), error)) {
        unsavedChanges_ = false;
    } else {
        unsavedChanges_ = true;
        out_ << "Warning: could not save your changes: " << error << '\n'
             << "They are kept in memory and the program will try to save again.\n";
    }
}

// ------------------------------------------------------------- output ----

void Cli::report(const OperationResult& result) {
    out_ << (result.ok ? "" : "Error: ") << result.message << '\n';
}

std::string Cli::statusOf(const std::string& bookId) const {
    const std::optional<Loan> loan = library_.findLoan(bookId);
    if (!loan) {
        return "Available";
    }
    return loan->isOverdue(today_) ? "Overdue" : "Borrowed";
}

void Cli::printBooks(const std::vector<Book>& books) {
    std::vector<std::vector<std::string>> rows;
    for (const Book& book : books) {
        rows.push_back({book.id(), textutil::truncate(book.title(), kTitleWidth),
                        textutil::truncate(book.author(), kAuthorWidth),
                        textutil::truncate(book.category(), kCategoryWidth),
                        std::to_string(book.year()), statusOf(book.id())});
    }
    printTable(out_, {"ID", "Title", "Author", "Category", "Year", "Status"}, rows);
}

void Cli::printBookDetails(const Book& book) {
    out_ << "Book " << book.id() << '\n'
         << "  Title:    " << book.title() << '\n'
         << "  Author:   " << book.author() << '\n'
         << "  Category: " << book.category() << '\n'
         << "  Year:     " << book.year() << '\n';
    const std::optional<Loan> loan = library_.findLoan(book.id());
    if (!loan) {
        out_ << "  Status:   Available\n";
        return;
    }
    out_ << "  Status:   Borrowed by " << loan->memberId();
    const std::optional<Member> member = library_.findMember(loan->memberId());
    if (member) {
        out_ << " (" << member->name() << ")";
    }
    out_ << ", due " << loan->dueDate().toString();
    if (loan->isOverdue(today_)) {
        out_ << " - OVERDUE by " << loan->daysOverdue(today_) << " day(s)";
    }
    out_ << '\n';
}

// -------------------------------------------------------------- input ----

std::string Cli::readLine(const std::string& prompt) {
    out_ << prompt << std::flush;
    std::string line;
    if (!std::getline(in_, line)) {
        out_ << '\n';
        throw InputEnded{};
    }
    return line;
}

void Cli::askWithRetry(const std::string& prompt,
                       const std::function<bool(const std::string&, std::string&)>& accept) {
    for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
        const std::string line = readLine(prompt);
        std::string error;
        if (accept(line, error)) {
            return;
        }
        out_ << "  Error: " << error << '\n';
    }
    throw InputCancelled{};
}

std::string Cli::askId(const std::string& prompt, const std::string& label,
                       const std::function<bool(const std::string&)>& exists, IdRule rule,
                       const std::string& hint) {
    std::string result;
    const std::string noun = textutil::toLower(label.substr(0, label.find(' ')));  // "book" / "member"
    askWithRetry(prompt, [&](const std::string& line, std::string& error) {
        const std::string id = validation::normalizeId(line);
        error = validation::checkId(id, label);
        if (!error.empty()) {
            return false;
        }
        if (rule == IdRule::MustBeNew && exists(id)) {
            error = "A " + noun + " with ID " + id + " already exists.";
            return false;
        }
        if (rule == IdRule::MustExist && !exists(id)) {
            error = "No " + noun + " with ID " + id + ". " + hint;
            return false;
        }
        result = id;
        return true;
    });
    return result;
}

std::string Cli::askBookId(const std::string& prompt, IdRule rule) {
    return askId(prompt, "Book ID",
                 [this](const std::string& id) { return library_.hasBook(id); }, rule,
                 "Choose option 1 to see all book IDs.");
}

std::string Cli::askMemberId(const std::string& prompt, IdRule rule) {
    return askId(prompt, "Member ID",
                 [this](const std::string& id) { return library_.hasMember(id); }, rule,
                 "Choose option 8 to see all member IDs.");
}

std::string Cli::askText(const std::string& prompt, const std::string& label) {
    std::string result;
    askWithRetry(prompt, [&](const std::string& line, std::string& error) {
        const std::string text = textutil::normalizeSpaces(line);
        error = validation::checkText(text, label);
        if (!error.empty()) {
            return false;
        }
        result = text;
        return true;
    });
    return result;
}

// Pressing Enter on its own means "keep the current value".
std::optional<std::string> Cli::askOptionalText(const std::string& prompt,
                                                const std::string& label) {
    std::optional<std::string> result;
    askWithRetry(prompt, [&](const std::string& line, std::string& error) {
        const std::string text = textutil::normalizeSpaces(line);
        if (text.empty()) {
            result.reset();
            return true;
        }
        error = validation::checkText(text, label);
        if (!error.empty()) {
            return false;
        }
        result = text;
        return true;
    });
    return result;
}

int Cli::askYear(const std::string& prompt) {
    int result = 0;
    askWithRetry(prompt, [&](const std::string& line, std::string& error) {
        int year = 0;
        if (!textutil::parseInteger(textutil::normalizeSpaces(line), year)) {
            error = "Year must be a whole number such as 2008.";
            return false;
        }
        error = validation::checkYear(year);
        if (!error.empty()) {
            return false;
        }
        result = year;
        return true;
    });
    return result;
}

std::optional<int> Cli::askOptionalYear(const std::string& prompt) {
    std::optional<int> result;
    askWithRetry(prompt, [&](const std::string& line, std::string& error) {
        const std::string text = textutil::normalizeSpaces(line);
        if (text.empty()) {
            result.reset();
            return true;
        }
        int year = 0;
        if (!textutil::parseInteger(text, year)) {
            error = "Year must be a whole number such as 2008.";
            return false;
        }
        error = validation::checkYear(year);
        if (!error.empty()) {
            return false;
        }
        result = year;
        return true;
    });
    return result;
}

// ------------------------------------------------------------ actions ----

void Cli::listBooks() {
    out_ << "\n--- All Books ---\n";
    if (library_.bookCount() == 0) {
        out_ << "The library has no books yet. Choose option 4 to add one, or option 12 to "
                "import a CSV file.\n";
        return;
    }
    printBooks(library_.books());
    out_ << library_.bookCount() << " book(s), " << library_.activeLoanCount()
         << " currently borrowed.\n";
}

void Cli::searchBooks() {
    out_ << "\n--- Search Books ---\n";
    std::string query;
    askWithRetry("Search text (ID, title, author, category or year): ",
                 [&](const std::string& line, std::string& error) {
                     const std::string text = textutil::normalizeSpaces(line);
                     error = validation::checkText(text, "Search text");
                     if (!error.empty()) {
                         return false;
                     }
                     query = text;
                     return true;
                 });
    const std::vector<Book> matches = library_.searchBooks(query);
    if (matches.empty()) {
        out_ << "No books match \"" << query << "\".\n";
        return;
    }
    out_ << matches.size() << " book(s) found:\n";
    printBooks(matches);
}

void Cli::checkAvailability() {
    out_ << "\n--- Check Availability ---\n";
    const std::string id = askBookId("Book ID: ", IdRule::MustExist);
    const std::optional<Book> book = library_.findBook(id);
    if (book) {
        printBookDetails(*book);
    }
}

void Cli::addBook() {
    out_ << "\n--- Add a Book ---\n";
    const std::string id = askBookId("New book ID (e.g. B007): ", IdRule::MustBeNew);
    const std::string title = askText("Title: ", "Title");
    const std::string author = askText("Author: ", "Author");
    const std::string category = askText("Category: ", "Category");
    const int year = askYear("Year published: ");
    const OperationResult result = library_.addBook(id, title, author, category, year);
    report(result);
    if (result.ok) {
        persist();
    }
}

void Cli::updateBook() {
    out_ << "\n--- Update a Book ---\n";
    const std::string id = askBookId("Book ID to update: ", IdRule::MustExist);
    const std::optional<Book> current = library_.findBook(id);
    if (!current) {
        return;
    }
    out_ << "Press Enter to keep the current value.\n";
    BookUpdate changes;
    changes.title = askOptionalText("Title [" + current->title() + "]: ", "Title");
    changes.author = askOptionalText("Author [" + current->author() + "]: ", "Author");
    changes.category = askOptionalText("Category [" + current->category() + "]: ", "Category");
    changes.year = askOptionalYear("Year [" + std::to_string(current->year()) + "]: ");

    if (!changes.title && !changes.author && !changes.category && !changes.year) {
        out_ << "No changes entered; the book was not modified.\n";
        return;
    }
    const OperationResult result = library_.updateBook(id, changes);
    report(result);
    if (result.ok) {
        persist();
    }
}

void Cli::addMember() {
    out_ << "\n--- Add a Member ---\n";
    const std::string id = askMemberId("New member ID (e.g. M030): ", IdRule::MustBeNew);
    const std::string name = askText("Member name: ", "Name");
    const OperationResult result = library_.addMember(id, name);
    report(result);
    if (result.ok) {
        persist();
    }
}

void Cli::renameMember() {
    out_ << "\n--- Update a Member's Name ---\n";
    const std::string id = askMemberId("Member ID: ", IdRule::MustExist);
    const std::optional<Member> current = library_.findMember(id);
    if (!current) {
        return;
    }
    const std::string name = askText("New name [" + current->name() + "]: ", "Name");
    const OperationResult result = library_.renameMember(id, name);
    report(result);
    if (result.ok) {
        persist();
    }
}

void Cli::listMembers() {
    out_ << "\n--- Members ---\n";
    const std::vector<Member> members = library_.members();
    if (members.empty()) {
        out_ << "There are no members yet. Choose option 6 to add one.\n";
        return;
    }
    std::vector<std::vector<std::string>> rows;
    for (const Member& member : members) {
        rows.push_back({member.id(), textutil::truncate(member.name(), kNameWidth),
                        std::to_string(library_.loanCountFor(member.id()))});
    }
    printTable(out_, {"ID", "Name", "Active loans"}, rows);
}

void Cli::issueBook() {
    out_ << "\n--- Issue (Borrow) a Book ---\n";
    const std::string bookId = askBookId("Book ID: ", IdRule::MustExist);
    const std::string memberId = askMemberId("Member ID: ", IdRule::MustExist);
    const OperationResult result = library_.issueBook(bookId, memberId, today_);
    report(result);
    if (result.ok) {
        persist();
    }
}

void Cli::returnBook() {
    out_ << "\n--- Return a Book ---\n";
    const std::string bookId = askBookId("Book ID: ", IdRule::MustExist);
    const OperationResult result = library_.returnBook(bookId, today_);
    report(result);
    if (result.ok) {
        persist();
    }
}

void Cli::listLoans() {
    out_ << "\n--- Active Loans ---\n";
    const std::vector<Loan> loans = library_.loans();
    if (loans.empty()) {
        out_ << "No books are currently borrowed.\n";
        return;
    }
    std::vector<std::vector<std::string>> rows;
    for (const Loan& loan : loans) {
        const std::optional<Book> book = library_.findBook(loan.bookId());
        const std::optional<Member> member = library_.findMember(loan.memberId());
        std::string status = "On time";
        if (loan.isOverdue(today_)) {
            status = "OVERDUE by " + std::to_string(loan.daysOverdue(today_)) + " day(s)";
        } else if (loan.dueDate() == today_) {
            status = "Due today";
        }
        rows.push_back({loan.bookId(),
                        book ? textutil::truncate(book->title(), kTitleWidth) : "?",
                        loan.memberId() + (member ? " " + textutil::truncate(member->name(), 16) : ""),
                        loan.dueDate().toString(), status});
    }
    printTable(out_, {"Book", "Title", "Member", "Due", "Status"}, rows);
}

void Cli::importCsv() {
    out_ << "\n--- Import Books from a CSV File ---\n";
    std::string path;
    askWithRetry("Path to the CSV file: ", [&](const std::string& line, std::string& error) {
        const std::string text = stripSurroundingQuotes(textutil::trim(line));
        if (text.empty()) {
            error = "Please enter a file path.";
            return false;
        }
        path = text;
        return true;
    });

    const BooksFile file = Storage::readBooksFile(path);
    if (!file.found) {
        out_ << "Error: file not found: " << path << '\n';
        return;
    }
    reportReadProblems(path, file.errors);
    if (file.books.empty() && !file.errors.empty()) {
        out_ << "Nothing was imported.\n";
        return;
    }

    const ImportSummary summary = library_.importBooks(file.books, file.loans);
    for (const std::string& note : summary.notes) {
        out_ << "Note: " << note << '\n';
    }
    out_ << "Import finished: " << summary.booksAdded << " book(s) added, "
         << summary.booksSkipped << " skipped, " << summary.loansAdded << " loan(s) added, "
         << file.errors.size() << " malformed row(s) ignored.\n";
    if (summary.booksAdded > 0 || summary.placeholderMembers > 0) {
        persist();
    }
}
