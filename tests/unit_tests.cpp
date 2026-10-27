// Unit tests for the Library Manager. No test framework is needed: this file
// has its own tiny CHECK macros.
//
// Build and run from the project folder (all on one line):
//   g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Isrc
//       src/Book.cpp src/CsvFormat.cpp src/Date.cpp src/Library.cpp src/Loan.cpp
//       src/Member.cpp src/Storage.cpp src/TextUtil.cpp src/Validation.cpp
//       tests/unit_tests.cpp -o unit_tests
// or simply:  make unit_tests   and then   ./unit_tests
//
// It must be run from the project folder because one test reads
// data/library_records_seed.csv.

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "Book.h"
#include "CsvFormat.h"
#include "Date.h"
#include "Library.h"
#include "Loan.h"
#include "Member.h"
#include "Storage.h"
#include "TextUtil.h"
#include "Validation.h"

namespace fs = std::filesystem;

// ----------------------------------------------------------- mini framework --

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(condition)                                                           \
    do {                                                                           \
        ++g_checks;                                                                \
        if (!(condition)) {                                                        \
            ++g_failures;                                                          \
            std::cerr << "  FAILED " << __FILE__ << ":" << __LINE__ << "  " #condition "\n"; \
        }                                                                          \
    } while (0)

template <typename A, typename B>
void checkEqual(const A& actual, const B& expected, const char* expression, int line) {
    ++g_checks;
    if (!(actual == expected)) {
        ++g_failures;
        std::cerr << "  FAILED line " << line << ": " << expression << "\n      actual:   "
                  << actual << "\n      expected: " << expected << "\n";
    }
}
#define CHECK_EQ(actual, expected) checkEqual((actual), (expected), #actual " == " #expected, __LINE__)

static bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

// A temporary folder that is deleted when the test ends.
class TempDir {
public:
    TempDir() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = fs::temp_directory_path() / ("library_manager_test_" + std::to_string(stamp));
        fs::create_directories(path_);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path_, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
    const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

static void writeFile(const fs::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

static std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

static Date D(int y, int m, int d) {
    return *Date::fromYmd(y, m, d);
}

// ------------------------------------------------------------------- Date ----

static void testDate() {
    const std::optional<Date> parsed = Date::parse("2026-09-05");
    CHECK(parsed.has_value());
    CHECK_EQ(parsed->year(), 2026);
    CHECK_EQ(parsed->month(), 9);
    CHECK_EQ(parsed->day(), 5);
    CHECK_EQ(parsed->toString(), "2026-09-05");

    // Invalid dates must be rejected.
    const std::vector<std::string> bad = {
        "",           "2026-9-5",   "2026/09/05", "20260905",   "2026-13-01", "2026-00-10",
        "2026-09-00", "2026-02-30", "2023-02-29", "2026-04-31", "abcd-ef-gh", "2026-09-05 ",
        " 2026-09-05", "0999-01-01", "1900-02-29", "2100-02-29", "2026-09-5x"};
    for (const std::string& text : bad) {
        CHECK(!Date::parse(text).has_value());
    }

    // Leap-day boundaries.
    CHECK(Date::parse("2024-02-29").has_value());
    CHECK(Date::parse("2000-02-29").has_value());
    CHECK(Date::isLeapYear(2024) && Date::isLeapYear(2000));
    CHECK(!Date::isLeapYear(2100) && !Date::isLeapYear(2023) && !Date::isLeapYear(1900));
    CHECK_EQ(Date::daysInMonth(2024, 2), 29);
    CHECK_EQ(Date::daysInMonth(2023, 2), 28);
    CHECK_EQ(Date::daysInMonth(2026, 4), 30);
    CHECK_EQ(Date::daysInMonth(2026, 12), 31);

    // Adding days across month, year and leap-day boundaries.
    CHECK_EQ(D(2026, 12, 31).addDays(1).toString(), "2027-01-01");
    CHECK_EQ(D(2024, 2, 28).addDays(1).toString(), "2024-02-29");
    CHECK_EQ(D(2024, 2, 29).addDays(1).toString(), "2024-03-01");
    CHECK_EQ(D(2023, 2, 28).addDays(1).toString(), "2023-03-01");
    CHECK_EQ(D(2026, 9, 8).addDays(14).toString(), "2026-09-22");
    CHECK_EQ(D(2026, 1, 20).addDays(14).toString(), "2026-02-03");
    CHECK_EQ(D(2026, 12, 25).addDays(14).toString(), "2027-01-08");
    CHECK_EQ(D(2026, 3, 1).addDays(-1).toString(), "2026-02-28");
    CHECK_EQ(D(2024, 3, 1).addDays(-1).toString(), "2024-02-29");
    CHECK_EQ(D(2026, 5, 5).addDays(0).toString(), "2026-05-05");

    // Differences.
    CHECK_EQ(D(2026, 9, 5).daysUntil(D(2026, 9, 8)), 3);
    CHECK_EQ(D(2026, 9, 8).daysUntil(D(2026, 9, 5)), -3);
    CHECK_EQ(D(2026, 12, 31).daysUntil(D(2027, 1, 1)), 1);
    CHECK_EQ(D(2024, 2, 28).daysUntil(D(2024, 3, 1)), 2);
    CHECK_EQ(D(2023, 2, 28).daysUntil(D(2023, 3, 1)), 1);
    CHECK_EQ(D(2026, 1, 1).daysUntil(D(2027, 1, 1)), 365);
    CHECK_EQ(D(2024, 1, 1).daysUntil(D(2025, 1, 1)), 366);

    // Comparisons and the clock.
    CHECK(D(2026, 9, 5) < D(2026, 9, 6));
    CHECK(D(2026, 9, 5) == D(2026, 9, 5));
    CHECK(D(2026, 9, 5) <= D(2026, 9, 5) && D(2026, 9, 5) >= D(2026, 9, 5));
    CHECK(D(2027, 1, 1) > D(2026, 12, 31) && D(2026, 12, 31) != D(2027, 1, 1));
    const Date now = Date::today();
    CHECK(Date::parse(now.toString()).has_value());
    CHECK(*Date::parse(now.toString()) == now);
}

static void testLoanBoundaries() {
    const Loan loan("B001", "M001", D(2026, 9, 5));
    CHECK(!loan.isOverdue(D(2026, 9, 4)));
    CHECK(!loan.isOverdue(D(2026, 9, 5)));  // the due date itself is NOT late
    CHECK(loan.isOverdue(D(2026, 9, 6)));   // one day after is late
    CHECK_EQ(loan.daysOverdue(D(2026, 9, 5)), 0);
    CHECK_EQ(loan.daysOverdue(D(2026, 9, 6)), 1);
    CHECK_EQ(loan.daysOverdue(D(2026, 9, 8)), 3);
    CHECK_EQ(loan.daysOverdue(D(2026, 8, 1)), 0);
    CHECK_EQ(loan.bookId(), "B001");
    CHECK_EQ(loan.memberId(), "M001");
}

// ------------------------------------------------------------------- text ----

static void testTextUtil() {
    CHECK_EQ(textutil::normalizeSpaces("  a   b \t c  "), "a b c");
    CHECK_EQ(textutil::normalizeSpaces("   "), "");
    CHECK_EQ(textutil::trim("  hi there \r\n"), "hi there");
    CHECK_EQ(textutil::toUpper("b001"), "B001");
    CHECK_EQ(textutil::toLower("AbC"), "abc");
    CHECK(textutil::containsIgnoreCase("Clean Code", "CLEAN"));
    CHECK(textutil::containsIgnoreCase("abc", ""));
    CHECK(!textutil::containsIgnoreCase("abc", "d"));
    CHECK(textutil::hasControlCharacter(std::string("a\x01" "b")));
    CHECK(!textutil::hasControlCharacter("plain text"));

    int value = -1;
    CHECK(textutil::parseInteger("2008", value) && value == 2008);
    CHECK(textutil::parseInteger("0", value) && value == 0);
    for (const char* bad : {"", "-5", "+5", "12a", "1.5", " 7", "1234567890", "abc"}) {
        CHECK(!textutil::parseInteger(bad, value));
    }

    CHECK_EQ(textutil::truncate("short", 10), "short");
    CHECK_EQ(textutil::truncate("abcdefghij", 8), "abcde...");
    CHECK_EQ(textutil::truncate("\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9", 8),  // 5 x "e-acute"
             "\xC3\xA9\xC3\xA9...");  // never cuts inside a 2-byte character
    CHECK_EQ(textutil::padRight("ab", 4), "ab  ");
    CHECK_EQ(textutil::padRight("abcdef", 4), "abcdef");
}

static void testValidation() {
    CHECK_EQ(validation::normalizeId("  b001 "), "B001");
    CHECK(validation::checkId("B001", "Book ID").empty());
    CHECK(validation::checkId("A-b_9", "Book ID").empty());
    CHECK(validation::checkId("ABCDEFGHIJKL", "Book ID").empty());  // 12 = allowed
    CHECK(!validation::checkId("ABCDEFGHIJKLM", "Book ID").empty());  // 13 = too long
    CHECK(!validation::checkId("", "Book ID").empty());
    CHECK(!validation::checkId("B 01", "Book ID").empty());
    CHECK(!validation::checkId("B,01", "Book ID").empty());
    CHECK(!validation::checkId("B\"01", "Book ID").empty());

    CHECK(validation::checkText("Clean Code", "Title").empty());
    CHECK(!validation::checkText("", "Title").empty());
    CHECK(validation::checkText(std::string(100, 'x'), "Title").empty());
    CHECK(!validation::checkText(std::string(101, 'x'), "Title").empty());
    CHECK(!validation::checkText(std::string("a\x07" "b"), "Title").empty());

    CHECK(!validation::checkYear(999).empty());
    CHECK(validation::checkYear(1000).empty());
    CHECK(validation::checkYear(2100).empty());
    CHECK(!validation::checkYear(2101).empty());
}

static void testCsv() {
    std::vector<std::string> fields;
    std::string error;

    CHECK(csv::parseLine("a,b,c", fields, error));
    CHECK_EQ(fields.size(), 3U);

    CHECK(csv::parseLine("B001,Title,Auth,Cat,1999,available,,", fields, error));
    CHECK_EQ(fields.size(), 8U);
    CHECK_EQ(fields[6], "");
    CHECK_EQ(fields[7], "");

    CHECK(csv::parseLine("\"Smith, John\",x", fields, error));
    CHECK_EQ(fields.size(), 2U);
    CHECK_EQ(fields[0], "Smith, John");

    CHECK(csv::parseLine("\"He said \"\"hi\"\"\",y", fields, error));
    CHECK_EQ(fields[0], "He said \"hi\"");

    CHECK(csv::parseLine("", fields, error));
    CHECK_EQ(fields.size(), 1U);

    CHECK(csv::parseLine("\"\",z", fields, error));
    CHECK_EQ(fields[0], "");

    CHECK(!csv::parseLine("\"abc", fields, error));
    CHECK(contains(error, "unterminated"));
    CHECK(!csv::parseLine("ab\"c", fields, error));
    CHECK(!csv::parseLine("\"abc\"x", fields, error));

    // Writing: escape, then parse back = the original fields.
    const std::vector<std::string> original = {"a,b", "say \"hi\"", " lead", "plain", "", "trail "};
    const std::string line = csv::joinLine(original);
    CHECK(csv::parseLine(line, fields, error));
    CHECK(fields == original);
    CHECK_EQ(csv::escapeField("plain"), "plain");
    CHECK_EQ(csv::escapeField("a,b"), "\"a,b\"");
    CHECK_EQ(csv::escapeField("q\"q"), "\"q\"\"q\"");
}

// ---------------------------------------------------------------- Library ----

static void testLibraryBooks() {
    Library lib;
    CHECK_EQ(lib.bookCount(), 0U);
    CHECK(lib.books().empty());
    CHECK(lib.searchBooks("anything").empty());  // empty collection is fine
    CHECK(lib.loans().empty());
    CHECK(!lib.isAvailable("B001"));  // unknown book

    CHECK(lib.addBook("b001", "  Clean   Code ", "Robert C Martin", "Software", 2008).ok);
    CHECK_EQ(lib.bookCount(), 1U);
    CHECK_EQ(lib.findBook("B001")->title(), "Clean Code");  // spaces tidied, ID upper-cased
    CHECK(lib.hasBook("b001"));

    // Duplicate IDs are refused (case-insensitively) and change nothing.
    const OperationResult dup = lib.addBook("B001", "Other", "Someone", "Misc", 2000);
    CHECK(!dup.ok);
    CHECK(contains(dup.message, "already exists"));
    CHECK(!lib.addBook(" b001 ", "Other", "Someone", "Misc", 2000).ok);
    CHECK_EQ(lib.findBook("B001")->title(), "Clean Code");
    CHECK_EQ(lib.bookCount(), 1U);

    // Invalid fields.
    CHECK(!lib.addBook("", "T", "A", "C", 2000).ok);
    CHECK(!lib.addBook("B 2", "T", "A", "C", 2000).ok);
    CHECK(!lib.addBook("B002", "", "A", "C", 2000).ok);
    CHECK(!lib.addBook("B002", "T", "   ", "C", 2000).ok);
    CHECK(!lib.addBook("B002", "T", "A", "", 2000).ok);
    CHECK(!lib.addBook("B002", "T", "A", "C", 999).ok);
    CHECK(!lib.addBook("B002", "T", "A", "C", 2101).ok);
    CHECK_EQ(lib.bookCount(), 1U);

    // Updating: atomic (a bad field means NOTHING changes).
    BookUpdate changes;
    CHECK(!lib.updateBook("B999", changes).ok);  // unknown book
    CHECK(!lib.updateBook("B001", changes).ok);  // nothing to change
    changes.title = "Clean Code, 2nd Edition";
    changes.year = 3000;  // invalid
    const OperationResult bad = lib.updateBook("B001", changes);
    CHECK(!bad.ok);
    CHECK_EQ(lib.findBook("B001")->title(), "Clean Code");
    CHECK_EQ(lib.findBook("B001")->year(), 2008);
    changes.year = 2020;
    CHECK(lib.updateBook("b001", changes).ok);
    CHECK_EQ(lib.findBook("B001")->title(), "Clean Code, 2nd Edition");
    CHECK_EQ(lib.findBook("B001")->year(), 2020);
    CHECK_EQ(lib.findBook("B001")->author(), "Robert C Martin");  // untouched
    BookUpdate blank;
    blank.author = "   ";
    CHECK(!lib.updateBook("B001", blank).ok);
}

static void testLibraryMembers() {
    Library lib;
    CHECK(lib.members().empty());
    CHECK(lib.addMember("m001", "Alice Smith").ok);
    CHECK(!lib.addMember("M001", "Someone Else").ok);  // duplicate
    CHECK(!lib.addMember("", "Nobody").ok);
    CHECK(!lib.addMember("M002", "  ").ok);
    CHECK_EQ(lib.memberCount(), 1U);
    CHECK_EQ(lib.findMember("M001")->name(), "Alice Smith");
    CHECK(lib.renameMember("M001", "Alice  Jones").ok);
    CHECK_EQ(lib.findMember("m001")->name(), "Alice Jones");
    CHECK(!lib.renameMember("M404", "Ghost").ok);
    CHECK(!lib.renameMember("M001", "").ok);
    CHECK_EQ(lib.findMember("M001")->name(), "Alice Jones");
}

static void testLibraryLoans() {
    Library lib;
    lib.addBook("B001", "One", "A", "C", 2000);
    lib.addBook("B002", "Two", "A", "C", 2001);
    lib.addBook("B003", "Three", "A", "C", 2002);
    lib.addBook("B004", "Four", "A", "C", 2003);
    lib.addMember("M001", "Alice");
    lib.addMember("M002", "Bob");
    const Date today = D(2026, 9, 8);

    // Failure cases first.
    CHECK(!lib.issueBook("B404", "M001", today).ok);
    CHECK(!lib.issueBook("B001", "M404", today).ok);
    CHECK(!lib.returnBook("B404", today).ok);
    const OperationResult notBorrowed = lib.returnBook("B001", today);
    CHECK(!notBorrowed.ok);
    CHECK(contains(notBorrowed.message, "not currently borrowed"));
    CHECK(lib.isAvailable("B001"));

    // Issue.
    const OperationResult issued = lib.issueBook("b001", "m001", today);
    CHECK(issued.ok);
    CHECK(contains(issued.message, "2026-09-22"));  // today + 14 days
    CHECK(!lib.isAvailable("B001"));
    CHECK_EQ(lib.findLoan("B001")->memberId(), "M001");
    CHECK_EQ(lib.findLoan("B001")->dueDate().toString(), "2026-09-22");
    CHECK_EQ(lib.activeLoanCount(), 1U);

    // The same book cannot be issued twice, not even to the same member.
    const OperationResult twice = lib.issueBook("B001", "M002", today);
    CHECK(!twice.ok);
    CHECK(contains(twice.message, "already borrowed by M001"));
    CHECK(!lib.issueBook("B001", "M001", today).ok);

    // Limit of 3 loans per member.
    CHECK(lib.issueBook("B002", "M001", today).ok);
    CHECK(lib.issueBook("B003", "M001", today).ok);
    CHECK_EQ(lib.loanCountFor("M001"), 3U);
    const OperationResult limit = lib.issueBook("B004", "M001", today);
    CHECK(!limit.ok);
    CHECK(contains(limit.message, "limit"));
    CHECK(lib.isAvailable("B004"));
    CHECK(lib.issueBook("B004", "M002", today).ok);  // another member can still borrow

    // Returning frees the book and the member's slot.
    const OperationResult onTime = lib.returnBook("B001", today.addDays(5));
    CHECK(onTime.ok);
    CHECK(contains(onTime.message, "on time"));
    CHECK(lib.isAvailable("B001"));
    CHECK_EQ(lib.loanCountFor("M001"), 2U);
    CHECK(lib.issueBook("B001", "M002", today).ok);

    // Boundary: returning ON the due date is on time; one day later is late.
    const OperationResult onDueDate = lib.returnBook("B002", D(2026, 9, 22));
    CHECK(onDueDate.ok && contains(onDueDate.message, "on time"));
    const OperationResult oneLate = lib.returnBook("B003", D(2026, 9, 23));
    CHECK(oneLate.ok && contains(oneLate.message, "1 day(s) late"));
    const OperationResult veryLate = lib.returnBook("B004", D(2026, 10, 2));
    CHECK(veryLate.ok && contains(veryLate.message, "10 day(s) late"));
    CHECK(!lib.returnBook("B004", D(2026, 10, 2)).ok);  // already returned
}

static void testLibrarySearchAndOrdering() {
    Library lib;
    lib.addBook("B010", "Clean Code", "Robert C Martin", "Software", 2008);
    lib.addBook("B002", "Deep Work", "Cal Newport", "Productivity", 2016);
    lib.addBook("B001", "Clean Architecture", "Robert C Martin", "Software", 2017);

    CHECK_EQ(lib.books().front().id(), "B001");  // sorted by ID
    CHECK_EQ(lib.books().back().id(), "B010");

    CHECK_EQ(lib.searchBooks("clean").size(), 2U);          // title, case-insensitive
    CHECK_EQ(lib.searchBooks("MARTIN").size(), 2U);         // author
    CHECK_EQ(lib.searchBooks("productivity").size(), 1U);   // category
    CHECK_EQ(lib.searchBooks("2016").size(), 1U);           // year
    CHECK_EQ(lib.searchBooks("b002").size(), 1U);           // ID
    CHECK_EQ(lib.searchBooks("  deep   work ").size(), 1U); // spaces tidied
    CHECK(lib.searchBooks("zzz").empty());
    CHECK_EQ(lib.searchBooks("").size(), 3U);               // empty query = everything

    lib.addMember("M001", "A");
    lib.addMember("M002", "B");
    lib.issueBook("B010", "M001", D(2026, 9, 1));  // due 09-15
    lib.issueBook("B001", "M002", D(2026, 9, 1));  // due 09-15
    lib.issueBook("B002", "M001", D(2026, 8, 1));  // due 08-15
    const std::vector<Loan> loans = lib.loans();
    CHECK_EQ(loans.size(), 3U);
    CHECK_EQ(loans[0].bookId(), "B002");  // earliest due date first
    CHECK_EQ(loans[1].bookId(), "B001");  // then by book ID
    CHECK_EQ(loans[2].bookId(), "B010");
}

static void testLibraryImport() {
    Library lib;
    lib.addBook("B001", "Existing", "Me", "Mine", 2000);

    const std::vector<Book> books = {Book("B001", "Duplicate", "X", "Y", 1999),
                                     Book("B002", "New One", "X", "Y", 2001),
                                     Book("B003", "New Two", "X", "Y", 2002)};
    const std::vector<Loan> loans = {Loan("B001", "M001", D(2026, 9, 5)),  // book was a duplicate
                                     Loan("B002", "M009", D(2026, 9, 6))};  // unknown member
    const ImportSummary summary = lib.importBooks(books, loans);
    CHECK_EQ(summary.booksAdded, 2U);
    CHECK_EQ(summary.booksSkipped, 1U);
    CHECK_EQ(summary.loansAdded, 1U);
    CHECK_EQ(summary.placeholderMembers, 1U);
    CHECK_EQ(lib.findBook("B001")->title(), "Existing");  // existing record wins
    CHECK(lib.isAvailable("B001"));                       // duplicate's loan ignored
    CHECK(!lib.isAvailable("B002"));
    CHECK(lib.hasMember("M009"));                         // placeholder was created
    CHECK_EQ(lib.findMember("M009")->name(), "Unknown member");

    // Importing the same data again changes nothing.
    const ImportSummary again = lib.importBooks(books, loans);
    CHECK_EQ(again.booksAdded, 0U);
    CHECK_EQ(again.booksSkipped, 3U);
    CHECK_EQ(again.loansAdded, 0U);
    CHECK_EQ(lib.bookCount(), 3U);
    CHECK_EQ(lib.activeLoanCount(), 1U);

    // Members: duplicates and invalid entries are skipped.
    const ImportSummary members =
        lib.importMembers({Member("M009", "Dup"), Member("M010", "Fine"), Member("", "No ID")});
    CHECK_EQ(members.membersAdded, 1U);
    CHECK_EQ(members.membersSkipped, 2U);
    CHECK(lib.hasMember("M010"));
}

// ---------------------------------------------------------------- Storage ----

static const char* const kBooksHeaderLine =
    "book_id,title,author,category,year,status,borrower_id,due_date\n";

static void testStorageMissingAndEmpty() {
    TempDir dir;
    const BooksFile missing = Storage::readBooksFile(dir.path() / "nope.csv");
    CHECK(!missing.found);
    CHECK(missing.books.empty() && missing.errors.empty());
    CHECK(!Storage::readMembersFile(dir.path() / "nope.csv").found);

    writeFile(dir.path() / "empty.csv", "");
    const BooksFile empty = Storage::readBooksFile(dir.path() / "empty.csv");
    CHECK(empty.found && empty.books.empty() && empty.errors.empty());

    writeFile(dir.path() / "headeronly.csv", kBooksHeaderLine);
    const BooksFile headerOnly = Storage::readBooksFile(dir.path() / "headeronly.csv");
    CHECK(headerOnly.found && headerOnly.books.empty() && headerOnly.errors.empty());

    // A folder where a file is expected must be reported, not crash.
    fs::create_directories(dir.path() / "adir");
    const BooksFile asDir = Storage::readBooksFile(dir.path() / "adir");
    CHECK(asDir.found);
    CHECK_EQ(asDir.errors.size(), 1U);

    // Wrong header: nothing is trusted.
    writeFile(dir.path() / "wrong.csv", "foo,bar\nB001,x\n");
    const BooksFile wrong = Storage::readBooksFile(dir.path() / "wrong.csv");
    CHECK(wrong.books.empty());
    CHECK_EQ(wrong.errors.size(), 1U);
    CHECK(contains(wrong.errors[0].message, "header"));
}

static void testStorageSeedFile() {
    const BooksFile seed = Storage::readBooksFile("data/library_records_seed.csv");
    CHECK(seed.found);
    CHECK_EQ(seed.errors.size(), 0U);
    CHECK_EQ(seed.books.size(), 6U);
    CHECK_EQ(seed.loans.size(), 2U);
    CHECK_EQ(seed.books[0].id(), "B001");
    CHECK_EQ(seed.books[0].title(), "The Pragmatic Programmer");
    CHECK_EQ(seed.books[0].year(), 1999);
    bool foundB002 = false;
    for (const Loan& loan : seed.loans) {
        if (loan.bookId() == "B002") {
            foundB002 = true;
            CHECK_EQ(loan.memberId(), "M014");
            CHECK_EQ(loan.dueDate().toString(), "2026-09-05");
        }
    }
    CHECK(foundB002);
}

static void testStorageMalformedRows() {
    TempDir dir;
    const std::string content =
        std::string(kBooksHeaderLine) +
        "B001,Good Book,Auth,Cat,2000,available,,\n"             // line 2: fine
        "B002,Short row,Auth,Cat\n"                              // line 3: too few columns
        "B003,Bad year,Auth,Cat,abcd,available,,\n"              // line 4
        "B004,Bad date,Auth,Cat,2000,borrowed,M001,2026-02-30\n" // line 5: impossible date
        "B005,No borrower,Auth,Cat,2000,borrowed,,2026-09-05\n"  // line 6
        "B006,Avail+borrower,Auth,Cat,2000,available,M001,\n"    // line 7
        "B007,Bad status,Auth,Cat,2000,lost,,\n"                 // line 8
        "b001,Duplicate,Auth,Cat,2000,available,,\n"             // line 9: duplicate ID
        "\"B008,Unterminated,Auth,Cat,2000,available,,\n"        // line 10: bad quote
        "B009,Fine Two,Auth,Cat,1999,borrowed,M002,2026-09-05\n" // line 11: fine, with loan
        "\n"                                                     // line 12: blank (ignored)
        "B010,Year range,Auth,Cat,3000,available,,\n";           // line 13
    writeFile(dir.path() / "books.csv", content);

    const BooksFile file = Storage::readBooksFile(dir.path() / "books.csv");
    CHECK_EQ(file.books.size(), 2U);
    CHECK_EQ(file.loans.size(), 1U);
    CHECK_EQ(file.books[0].id(), "B001");
    CHECK_EQ(file.books[1].id(), "B009");
    CHECK_EQ(file.loans[0].bookId(), "B009");
    CHECK_EQ(file.errors.size(), 9U);
    if (file.errors.size() == 9U) {
        CHECK_EQ(file.errors[0].line, 3U);
        CHECK_EQ(file.errors[1].line, 4U);
        CHECK_EQ(file.errors[2].line, 5U);
        CHECK_EQ(file.errors[3].line, 6U);
        CHECK_EQ(file.errors[4].line, 7U);
        CHECK_EQ(file.errors[5].line, 8U);
        CHECK_EQ(file.errors[6].line, 9U);
        CHECK(contains(file.errors[6].message, "duplicate"));
        CHECK(contains(file.errors[6].message, "line 2"));
        CHECK_EQ(file.errors[7].line, 10U);
        CHECK_EQ(file.errors[8].line, 13U);
    }
}

static void testStorageBomAndCrlf() {
    TempDir dir;
    writeFile(dir.path() / "excel.csv",
              std::string("\xEF\xBB\xBF") + "book_id,title,author,category,year,status,borrower_id,due_date\r\n"
              "B001,Windows Book,Auth,Cat,2001,available,,\r\n"
              "B002,Other,Auth,Cat,2002,borrowed,m003,2026-09-05\r\n");
    const BooksFile file = Storage::readBooksFile(dir.path() / "excel.csv");
    CHECK_EQ(file.errors.size(), 0U);
    CHECK_EQ(file.books.size(), 2U);
    CHECK_EQ(file.books[0].title(), "Windows Book");
    CHECK_EQ(file.loans.size(), 1U);
    CHECK_EQ(file.loans[0].memberId(), "M003");  // normalised to upper-case
}

static void testStorageMembersFile() {
    TempDir dir;
    writeFile(dir.path() / "members.csv",
              "member_id,name\n"
              "M001,Alice Smith\n"
              "M002\n"                 // line 3: too few columns
              "M003,\n"                // line 4: empty name
              "m001,Duplicate\n"       // line 5: duplicate ID
              "M004,\"Jones, Bob\"\n"); // line 6: comma inside quotes is fine
    const MembersFile file = Storage::readMembersFile(dir.path() / "members.csv");
    CHECK_EQ(file.members.size(), 2U);
    CHECK_EQ(file.members[1].name(), "Jones, Bob");
    CHECK_EQ(file.errors.size(), 3U);
}

static void testStorageRoundTrip() {
    TempDir dir;
    Library lib;
    lib.addBook("B001", "Plain Title", "Plain Author", "Cat", 2000);
    lib.addBook("B002", "Title, with comma and \"quotes\"", "Author, Jr.", "Cat", 2010);
    lib.addBook("B003", "Borrowed One", "Auth", "Cat", 1999);
    lib.addMember("M001", "Alice Smith");
    lib.addMember("M002", "Jones, Bob");
    lib.issueBook("B003", "M002", D(2026, 9, 8));

    Storage writer(dir.path() / "data");  // folder does not exist yet: must be created
    std::string error;
    CHECK(writer.save(lib.books(), lib.members(), lib.loans(), error));
    CHECK(error.empty());
    CHECK(fs::exists(dir.path() / "data" / "books.csv"));
    CHECK(fs::exists(dir.path() / "data" / "members.csv"));
    CHECK(!fs::exists(dir.path() / "data" / "books.csv.tmp"));  // no leftovers
    CHECK(!fs::exists(dir.path() / "data" / "members.csv.tmp"));

    Storage reader(dir.path() / "data");
    const MembersFile membersFile = reader.loadMembers();
    const BooksFile booksFile = reader.loadBooks();
    CHECK(membersFile.errors.empty() && booksFile.errors.empty());
    Library restored;
    restored.importMembers(membersFile.members);
    restored.importBooks(booksFile.books, booksFile.loans);

    CHECK_EQ(restored.bookCount(), 3U);
    CHECK_EQ(restored.memberCount(), 2U);
    CHECK_EQ(restored.activeLoanCount(), 1U);
    CHECK_EQ(restored.findBook("B002")->title(), "Title, with comma and \"quotes\"");
    CHECK_EQ(restored.findBook("B002")->author(), "Author, Jr.");
    CHECK_EQ(restored.findMember("M002")->name(), "Jones, Bob");
    CHECK(!restored.isAvailable("B003"));
    CHECK_EQ(restored.findLoan("B003")->dueDate().toString(), "2026-09-22");
    CHECK_EQ(restored.findLoan("B003")->memberId(), "M002");

    // Saving twice over existing files works and stays identical.
    const std::string before = readFile(dir.path() / "data" / "books.csv");
    CHECK(writer.save(restored.books(), restored.members(), restored.loans(), error));
    CHECK_EQ(readFile(dir.path() / "data" / "books.csv"), before);
}

static void testStorageBackupAndFailure() {
    TempDir dir;
    const std::string damaged =
        std::string(kBooksHeaderLine) + "B001,Good,Auth,Cat,2000,available,,\nTHIS ROW IS BROKEN\n";
    writeFile(dir.path() / "books.csv", damaged);

    Storage storage(dir.path());
    const BooksFile loaded = storage.loadBooks();
    CHECK_EQ(loaded.books.size(), 1U);
    CHECK_EQ(loaded.errors.size(), 1U);

    // The first save after a damaged load must keep a copy of the original.
    std::string error;
    CHECK(storage.save({}, {}, {}, error));
    CHECK(fs::exists(dir.path() / "books.csv.bak"));
    CHECK_EQ(readFile(dir.path() / "books.csv.bak"), damaged);
    CHECK_EQ(readFile(dir.path() / "books.csv"), std::string(kBooksHeaderLine));

    // A clean file produces no backup.
    TempDir clean;
    Storage cleanStorage(clean.path());
    cleanStorage.loadBooks();
    CHECK(cleanStorage.save({}, {}, {}, error));
    CHECK(!fs::exists(clean.path() / "books.csv.bak"));

    // Saving somewhere impossible reports an error instead of crashing.
    TempDir blocked;
    writeFile(blocked.path() / "afile", "i am a file, not a folder");
    Storage impossible(blocked.path() / "afile" / "sub");
    CHECK(!impossible.save({}, {}, {}, error));
    CHECK(!error.empty());
}

// -------------------------------------------------------------------- main ----

int main() {
    struct Test {
        const char* name;
        void (*run)();
    };
    const std::vector<Test> tests = {
        {"Date", testDate},
        {"Loan boundaries", testLoanBoundaries},
        {"TextUtil", testTextUtil},
        {"Validation", testValidation},
        {"CSV format", testCsv},
        {"Library: books", testLibraryBooks},
        {"Library: members", testLibraryMembers},
        {"Library: loans", testLibraryLoans},
        {"Library: search and ordering", testLibrarySearchAndOrdering},
        {"Library: import", testLibraryImport},
        {"Storage: missing / empty / wrong files", testStorageMissingAndEmpty},
        {"Storage: supplied seed file", testStorageSeedFile},
        {"Storage: malformed rows", testStorageMalformedRows},
        {"Storage: BOM and CRLF", testStorageBomAndCrlf},
        {"Storage: members file", testStorageMembersFile},
        {"Storage: save/load round trip", testStorageRoundTrip},
        {"Storage: backup and failures", testStorageBackupAndFailure},
    };

    int failedTests = 0;
    for (const Test& test : tests) {
        const int failuresBefore = g_failures;
        test.run();
        const bool passed = (g_failures == failuresBefore);
        std::cout << (passed ? "PASS  " : "FAIL  ") << test.name << '\n';
        if (!passed) {
            ++failedTests;
        }
    }
    std::cout << "\n" << tests.size() - static_cast<std::size_t>(failedTests) << " of "
              << tests.size() << " test groups passed (" << g_checks << " checks, "
              << g_failures << " failed)\n";
    return g_failures == 0 ? 0 : 1;
}
