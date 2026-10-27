// Library: the BUSINESS-RULES layer.
//
// Responsibility: own the collections of books, members and active loans and
// enforce every rule (unique IDs, valid text, who may borrow what, due dates).
// It knows nothing about files or the terminal, so it can be tested on its own.
//
// Ownership (RAII, no raw pointers, no manual new/delete):
//   * Library owns every Book, Member and Loan BY VALUE inside std::map
//     containers. When the Library is destroyed, everything is released.
//   * Loan refers to its Book and Member by ID (a string), never by pointer,
//     so nothing can dangle.
//   * Query functions return COPIES (or std::optional copies), so callers can
//     never hold a reference into a container that later changes.
//   * Whether a book is borrowed is stored ONLY as the presence of a Loan.
#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Book.h"
#include "Date.h"
#include "Loan.h"
#include "Member.h"

// The outcome of an operation: it worked, or here is why it did not.
struct OperationResult {
    bool ok;
    std::string message;

    static OperationResult success(std::string message) { return {true, std::move(message)}; }
    static OperationResult failure(std::string message) { return {false, std::move(message)}; }
};

// The fields of a book that may be changed. An empty optional means "leave as is".
struct BookUpdate {
    std::optional<std::string> title;
    std::optional<std::string> author;
    std::optional<std::string> category;
    std::optional<int> year;
};

// What happened during a bulk import (loading a file).
struct ImportSummary {
    std::size_t booksAdded = 0;
    std::size_t booksSkipped = 0;
    std::size_t membersAdded = 0;
    std::size_t membersSkipped = 0;
    std::size_t loansAdded = 0;
    std::size_t placeholderMembers = 0;  // members created only because a loan named them
    std::vector<std::string> notes;      // human-readable details for the user
};

class Library {
public:
    static constexpr int kLoanPeriodDays = 14;
    static constexpr std::size_t kMaxLoansPerMember = 3;

    // --- changing data (each validates its input and reports success/failure) ---
    OperationResult addBook(const std::string& id, const std::string& title,
                            const std::string& author, const std::string& category, int year);
    OperationResult updateBook(const std::string& id, const BookUpdate& changes);
    OperationResult addMember(const std::string& id, const std::string& name);
    OperationResult renameMember(const std::string& id, const std::string& newName);
    OperationResult issueBook(const std::string& bookId, const std::string& memberId,
                              const Date& today);
    OperationResult returnBook(const std::string& bookId, const Date& today);

    // --- bulk loading (used for saved data and for CSV import) ---
    ImportSummary importMembers(const std::vector<Member>& members);
    ImportSummary importBooks(const std::vector<Book>& books, const std::vector<Loan>& loans);

    // --- questions (all return copies) ---
    bool hasBook(const std::string& id) const;
    bool hasMember(const std::string& id) const;
    std::optional<Book> findBook(const std::string& id) const;
    std::optional<Member> findMember(const std::string& id) const;
    std::optional<Loan> findLoan(const std::string& bookId) const;
    bool isAvailable(const std::string& bookId) const;  // false for unknown books
    std::size_t loanCountFor(const std::string& memberId) const;

    std::size_t bookCount() const { return books_.size(); }
    std::size_t memberCount() const { return members_.size(); }
    std::size_t activeLoanCount() const { return loansByBook_.size(); }

    std::vector<Book> books() const;      // sorted by ID
    std::vector<Member> members() const;  // sorted by ID
    std::vector<Loan> loans() const;      // sorted by due date, then book ID

    // Case-insensitive search of ID, title, author, category and year.
    // An empty query matches every book.
    std::vector<Book> searchBooks(const std::string& query) const;

private:
    std::map<std::string, Book> books_;
    std::map<std::string, Member> members_;
    std::map<std::string, Loan> loansByBook_;  // key = book ID -> at most one loan per book
};
