// Storage: the DATA layer. Reads and writes the library's CSV files.
//
// Safety rules this class follows:
//  * A missing file is normal (first run), not an error.
//  * A malformed row is skipped and REPORTED (with its line number) - it never
//    crashes the program and never produces a half-built object.
//  * Saving writes a temporary file first and then swaps it into place, so a
//    crash or full disk in the middle of a save cannot leave a half-written
//    data file.
//  * If a file had problem rows when it was loaded, the original is copied to
//    "<name>.bak" before it is overwritten, so nothing is silently lost.
//
// Files (inside the data directory):
//    books.csv    book_id,title,author,category,year,status,borrower_id,due_date
//                 (exactly the columns of the supplied seed file; a borrowed
//                  row carries the Loan: borrower_id + due_date)
//    members.csv  member_id,name
#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "Book.h"
#include "Loan.h"
#include "Member.h"

// One row (or the whole file, when line is 0 or 1) that could not be used.
struct RowError {
    std::size_t line;
    std::string message;
};

struct BooksFile {
    bool found = false;  // false when the file does not exist
    std::vector<Book> books;
    std::vector<Loan> loans;  // one per borrowed row
    std::vector<RowError> errors;
};

struct MembersFile {
    bool found = false;
    std::vector<Member> members;
    std::vector<RowError> errors;
};

class Storage {
public:
    explicit Storage(std::filesystem::path dataDir);

    // Reads any file in the books / members format (used for the saved data
    // AND for importing the seed file). Never throws.
    static BooksFile readBooksFile(const std::filesystem::path& path);
    static MembersFile readMembersFile(const std::filesystem::path& path);

    // Read this storage's own files (and remember whether a backup is needed).
    BooksFile loadBooks();
    MembersFile loadMembers();

    // Writes both files. Returns false and sets `error` on failure.
    bool save(const std::vector<Book>& books, const std::vector<Member>& members,
              const std::vector<Loan>& loans, std::string& error);

    const std::filesystem::path& dataDir() const { return dataDir_; }
    std::filesystem::path booksPath() const { return dataDir_ / "books.csv"; }
    std::filesystem::path membersPath() const { return dataDir_ / "members.csv"; }

private:
    std::filesystem::path dataDir_;
    bool backupBooksBeforeSave_ = false;
    bool backupMembersBeforeSave_ = false;
};
