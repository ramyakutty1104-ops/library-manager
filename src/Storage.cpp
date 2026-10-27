#include "Storage.h"

#include <fstream>
#include <map>
#include <optional>
#include <system_error>
#include <utility>

#include "CsvFormat.h"
#include "Date.h"
#include "TextUtil.h"
#include "Validation.h"

namespace fs = std::filesystem;

namespace {

const std::string kBooksHeader = "book_id,title,author,category,year,status,borrower_id,due_date";
const std::string kMembersHeader = "member_id,name";
constexpr std::size_t kBookColumns = 8;
constexpr std::size_t kMemberColumns = 2;

// ---------------------------------------------------------------- reading --

struct RawFile {
    bool found = false;
    std::string openError;  // non-empty when the file exists but cannot be read
    std::vector<std::pair<std::size_t, std::string>> lines;  // (line number, text)
};

// Reads a text file line by line. Tolerates a UTF-8 BOM (added by Excel) and
// both Unix and Windows line endings.
RawFile readRawFile(const fs::path& path) {
    RawFile raw;
    std::error_code ec;
    const bool exists = fs::exists(path, ec);
    if (ec) {
        raw.found = true;
        raw.openError = "cannot access the file: " + ec.message();
        return raw;
    }
    if (!exists) {
        return raw;
    }
    raw.found = true;
    if (!fs::is_regular_file(path, ec)) {
        raw.openError = "this path is not a regular file";
        return raw;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        raw.openError = "could not open the file for reading";
        return raw;
    }
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(in, line)) {
        ++lineNumber;
        if (lineNumber == 1 && line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
            line.erase(0, 3);
        }
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
            line.pop_back();
        }
        raw.lines.emplace_back(lineNumber, line);
    }
    return raw;
}

bool isBlank(const std::string& line) {
    return textutil::normalizeSpaces(line).empty();
}

// "Book_ID, Title" -> "book_id,title" so headers compare loosely.
std::string headerKey(const std::string& line) {
    std::string key;
    for (char c : line) {
        if (!textutil::isSpace(c)) {
            key += c;
        }
    }
    return textutil::toLower(key);
}

struct ParsedBookRow {
    Book book;
    std::optional<Loan> loan;
};

// Validates one books.csv row. Returns std::nullopt and fills `error` if bad.
std::optional<ParsedBookRow> parseBookRow(const std::vector<std::string>& f, std::string& error) {
    const std::string id = validation::normalizeId(f[0]);
    const std::string title = textutil::normalizeSpaces(f[1]);
    const std::string author = textutil::normalizeSpaces(f[2]);
    const std::string category = textutil::normalizeSpaces(f[3]);
    const std::string yearText = textutil::normalizeSpaces(f[4]);
    const std::string status = textutil::toLower(textutil::normalizeSpaces(f[5]));
    const std::string borrower = validation::normalizeId(f[6]);
    const std::string dueText = textutil::normalizeSpaces(f[7]);

    error = validation::checkId(id, "book_id");
    if (error.empty()) error = validation::checkText(title, "title");
    if (error.empty()) error = validation::checkText(author, "author");
    if (error.empty()) error = validation::checkText(category, "category");
    int year = 0;
    if (error.empty() && !textutil::parseInteger(yearText, year)) {
        error = "year must be a whole number";
    }
    if (error.empty()) error = validation::checkYear(year);
    if (error.empty() && status != "available" && status != "borrowed") {
        error = "status must be 'available' or 'borrowed'";
    }
    if (!error.empty()) {
        return std::nullopt;
    }

    if (status == "available") {
        if (!borrower.empty() || !dueText.empty()) {
            error = "an available book must not have a borrower_id or due_date";
            return std::nullopt;
        }
        return ParsedBookRow{Book(id, title, author, category, year), std::nullopt};
    }

    // status == "borrowed": needs a valid borrower and a real due date.
    error = validation::checkId(borrower, "borrower_id");
    if (!error.empty()) {
        return std::nullopt;
    }
    const std::optional<Date> due = Date::parse(dueText);
    if (!due) {
        error = "due_date must be a real calendar date written as YYYY-MM-DD";
        return std::nullopt;
    }
    return ParsedBookRow{Book(id, title, author, category, year), Loan(id, borrower, *due)};
}

// ---------------------------------------------------------------- writing --

std::string buildBooksCsv(const std::vector<Book>& books, const std::vector<Loan>& loans) {
    std::map<std::string, const Loan*> loanByBook;
    for (const Loan& loan : loans) {
        loanByBook[loan.bookId()] = &loan;
    }
    std::string text = kBooksHeader + "\n";
    for (const Book& book : books) {
        std::vector<std::string> fields = {book.id(), book.title(), book.author(),
                                           book.category(), std::to_string(book.year())};
        const auto it = loanByBook.find(book.id());
        if (it != loanByBook.end()) {
            fields.push_back("borrowed");
            fields.push_back(it->second->memberId());
            fields.push_back(it->second->dueDate().toString());
        } else {
            fields.push_back("available");
            fields.push_back("");
            fields.push_back("");
        }
        text += csv::joinLine(fields) + "\n";
    }
    return text;
}

std::string buildMembersCsv(const std::vector<Member>& members) {
    std::string text = kMembersHeader + "\n";
    for (const Member& member : members) {
        text += csv::joinLine({member.id(), member.name()}) + "\n";
    }
    return text;
}

// Writes `content` to "<target>.tmp" and then moves it over `target`.
bool writeFileAtomically(const fs::path& target, const std::string& content, std::string& error) {
    std::error_code ec;
    if (target.has_parent_path()) {
        fs::create_directories(target.parent_path(), ec);
        if (ec) {
            error = "could not create folder " + target.parent_path().string() + ": " + ec.message();
            return false;
        }
    }

    fs::path temp = target;
    temp += ".tmp";
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) {
            error = "could not open " + temp.string() + " for writing";
            return false;
        }
        out << content;
        out.flush();
        if (!out) {
            error = "could not write to " + temp.string() + " (is the disk full?)";
            out.close();
            fs::remove(temp, ec);
            return false;
        }
    }

    fs::rename(temp, target, ec);
    if (ec) {
        // Some platforms refuse to rename over an existing file; fall back to copying.
        std::error_code copyError;
        fs::copy_file(temp, target, fs::copy_options::overwrite_existing, copyError);
        fs::remove(temp, ec);
        if (copyError) {
            error = "could not replace " + target.string() + ": " + copyError.message();
            return false;
        }
    }
    return true;
}

bool writeWithBackup(const fs::path& target, const std::string& content, bool& needsBackup,
                     std::string& error) {
    std::error_code ec;
    if (needsBackup && fs::exists(target, ec)) {
        fs::path backup = target;
        backup += ".bak";
        fs::copy_file(target, backup, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            error = "could not create the backup " + backup.string() + ": " + ec.message() +
                    " (the original file was left untouched)";
            return false;
        }
    }
    needsBackup = false;
    return writeFileAtomically(target, content, error);
}

}  // namespace

// -------------------------------------------------------------- Storage ----

Storage::Storage(fs::path dataDir) : dataDir_(std::move(dataDir)) {}

BooksFile Storage::readBooksFile(const fs::path& path) {
    BooksFile result;
    const RawFile raw = readRawFile(path);
    result.found = raw.found;
    if (!raw.found) {
        return result;
    }
    if (!raw.openError.empty()) {
        result.errors.push_back({0, raw.openError});
        return result;
    }

    bool headerSeen = false;
    std::map<std::string, std::size_t> firstLineOfId;

    for (const auto& entry : raw.lines) {
        const std::size_t lineNumber = entry.first;
        const std::string& line = entry.second;
        if (isBlank(line)) {
            continue;
        }
        if (!headerSeen) {
            headerSeen = true;
            if (headerKey(line) != kBooksHeader) {
                result.errors.push_back(
                    {lineNumber, "unexpected header row; expected: " + kBooksHeader});
                return result;  // we do not trust any row of a file with a wrong header
            }
            continue;
        }

        std::vector<std::string> fields;
        std::string error;
        if (!csv::parseLine(line, fields, error)) {
            result.errors.push_back({lineNumber, "malformed row: " + error});
            continue;
        }
        if (fields.size() != kBookColumns) {
            result.errors.push_back({lineNumber, "expected " + std::to_string(kBookColumns) +
                                                     " columns but found " +
                                                     std::to_string(fields.size())});
            continue;
        }
        const std::optional<ParsedBookRow> parsed = parseBookRow(fields, error);
        if (!parsed) {
            result.errors.push_back({lineNumber, error});
            continue;
        }
        const std::string& id = parsed->book.id();
        const auto seen = firstLineOfId.find(id);
        if (seen != firstLineOfId.end()) {
            result.errors.push_back({lineNumber, "duplicate book_id " + id +
                                                     " (first seen on line " +
                                                     std::to_string(seen->second) + ")"});
            continue;
        }
        firstLineOfId.emplace(id, lineNumber);
        result.books.push_back(parsed->book);
        if (parsed->loan) {
            result.loans.push_back(*parsed->loan);
        }
    }
    return result;
}

MembersFile Storage::readMembersFile(const fs::path& path) {
    MembersFile result;
    const RawFile raw = readRawFile(path);
    result.found = raw.found;
    if (!raw.found) {
        return result;
    }
    if (!raw.openError.empty()) {
        result.errors.push_back({0, raw.openError});
        return result;
    }

    bool headerSeen = false;
    std::map<std::string, std::size_t> firstLineOfId;

    for (const auto& entry : raw.lines) {
        const std::size_t lineNumber = entry.first;
        const std::string& line = entry.second;
        if (isBlank(line)) {
            continue;
        }
        if (!headerSeen) {
            headerSeen = true;
            if (headerKey(line) != kMembersHeader) {
                result.errors.push_back(
                    {lineNumber, "unexpected header row; expected: " + kMembersHeader});
                return result;
            }
            continue;
        }

        std::vector<std::string> fields;
        std::string error;
        if (!csv::parseLine(line, fields, error)) {
            result.errors.push_back({lineNumber, "malformed row: " + error});
            continue;
        }
        if (fields.size() != kMemberColumns) {
            result.errors.push_back({lineNumber, "expected " + std::to_string(kMemberColumns) +
                                                     " columns but found " +
                                                     std::to_string(fields.size())});
            continue;
        }
        const std::string id = validation::normalizeId(fields[0]);
        const std::string name = textutil::normalizeSpaces(fields[1]);
        error = validation::checkId(id, "member_id");
        if (error.empty()) error = validation::checkText(name, "name");
        if (!error.empty()) {
            result.errors.push_back({lineNumber, error});
            continue;
        }
        const auto seen = firstLineOfId.find(id);
        if (seen != firstLineOfId.end()) {
            result.errors.push_back({lineNumber, "duplicate member_id " + id +
                                                     " (first seen on line " +
                                                     std::to_string(seen->second) + ")"});
            continue;
        }
        firstLineOfId.emplace(id, lineNumber);
        result.members.emplace_back(id, name);
    }
    return result;
}

BooksFile Storage::loadBooks() {
    BooksFile file = readBooksFile(booksPath());
    if (!file.errors.empty()) {
        backupBooksBeforeSave_ = true;  // protect the original from being overwritten
    }
    return file;
}

MembersFile Storage::loadMembers() {
    MembersFile file = readMembersFile(membersPath());
    if (!file.errors.empty()) {
        backupMembersBeforeSave_ = true;
    }
    return file;
}

bool Storage::save(const std::vector<Book>& books, const std::vector<Member>& members,
                   const std::vector<Loan>& loans, std::string& error) {
    error.clear();
    return writeWithBackup(booksPath(), buildBooksCsv(books, loans), backupBooksBeforeSave_, error) &&
           writeWithBackup(membersPath(), buildMembersCsv(members), backupMembersBeforeSave_, error);
}
