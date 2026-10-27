#include "Library.h"

#include <algorithm>
#include <set>
#include <utility>

#include "TextUtil.h"
#include "Validation.h"

namespace {

std::string quoted(const std::string& text) {
    return "\"" + text + "\"";
}

}  // namespace

// ------------------------------------------------------------ changing ----

OperationResult Library::addBook(const std::string& id, const std::string& title,
                                 const std::string& author, const std::string& category,
                                 int year) {
    const std::string bookId = validation::normalizeId(id);
    const std::string cleanTitle = textutil::normalizeSpaces(title);
    const std::string cleanAuthor = textutil::normalizeSpaces(author);
    const std::string cleanCategory = textutil::normalizeSpaces(category);

    std::string error = validation::checkId(bookId, "Book ID");
    if (error.empty()) error = validation::checkText(cleanTitle, "Title");
    if (error.empty()) error = validation::checkText(cleanAuthor, "Author");
    if (error.empty()) error = validation::checkText(cleanCategory, "Category");
    if (error.empty()) error = validation::checkYear(year);
    if (!error.empty()) {
        return OperationResult::failure(error);
    }
    if (books_.count(bookId) != 0) {
        return OperationResult::failure("A book with ID " + bookId + " already exists.");
    }

    books_.emplace(bookId, Book(bookId, cleanTitle, cleanAuthor, cleanCategory, year));
    return OperationResult::success("Added book " + bookId + " (" + quoted(cleanTitle) + ").");
}

OperationResult Library::updateBook(const std::string& id, const BookUpdate& changes) {
    const std::string bookId = validation::normalizeId(id);
    const auto it = books_.find(bookId);
    if (it == books_.end()) {
        return OperationResult::failure("No book with ID " + bookId + ".");
    }
    if (!changes.title && !changes.author && !changes.category && !changes.year) {
        return OperationResult::failure("No changes were provided.");
    }

    // Check EVERYTHING first, so a bad value never leaves the book half-updated.
    std::string title, author, category;
    auto checkField = [](const std::optional<std::string>& value, const std::string& label,
                         std::string& cleaned) -> std::string {
        if (!value) {
            return "";
        }
        cleaned = textutil::normalizeSpaces(*value);
        return validation::checkText(cleaned, label);
    };
    std::string error = checkField(changes.title, "Title", title);
    if (error.empty()) error = checkField(changes.author, "Author", author);
    if (error.empty()) error = checkField(changes.category, "Category", category);
    if (error.empty() && changes.year) error = validation::checkYear(*changes.year);
    if (!error.empty()) {
        return OperationResult::failure(error);
    }

    Book& book = it->second;
    if (changes.title) book.setTitle(title);
    if (changes.author) book.setAuthor(author);
    if (changes.category) book.setCategory(category);
    if (changes.year) book.setYear(*changes.year);
    return OperationResult::success("Updated book " + bookId + ".");
}

OperationResult Library::addMember(const std::string& id, const std::string& name) {
    const std::string memberId = validation::normalizeId(id);
    const std::string cleanName = textutil::normalizeSpaces(name);

    std::string error = validation::checkId(memberId, "Member ID");
    if (error.empty()) error = validation::checkText(cleanName, "Name");
    if (!error.empty()) {
        return OperationResult::failure(error);
    }
    if (members_.count(memberId) != 0) {
        return OperationResult::failure("A member with ID " + memberId + " already exists.");
    }

    members_.emplace(memberId, Member(memberId, cleanName));
    return OperationResult::success("Added member " + memberId + " (" + cleanName + ").");
}

OperationResult Library::renameMember(const std::string& id, const std::string& newName) {
    const std::string memberId = validation::normalizeId(id);
    const auto it = members_.find(memberId);
    if (it == members_.end()) {
        return OperationResult::failure("No member with ID " + memberId + ".");
    }
    const std::string cleanName = textutil::normalizeSpaces(newName);
    const std::string error = validation::checkText(cleanName, "Name");
    if (!error.empty()) {
        return OperationResult::failure(error);
    }
    it->second.setName(cleanName);
    return OperationResult::success("Member " + memberId + " is now named " + cleanName + ".");
}

OperationResult Library::issueBook(const std::string& bookId, const std::string& memberId,
                                   const Date& today) {
    const std::string bid = validation::normalizeId(bookId);
    const std::string mid = validation::normalizeId(memberId);

    const auto bookIt = books_.find(bid);
    if (bookIt == books_.end()) {
        return OperationResult::failure("No book with ID " + bid + ".");
    }
    const auto memberIt = members_.find(mid);
    if (memberIt == members_.end()) {
        return OperationResult::failure("No member with ID " + mid + ".");
    }
    const auto loanIt = loansByBook_.find(bid);
    if (loanIt != loansByBook_.end()) {
        return OperationResult::failure("Book " + bid + " is already borrowed by " +
                                        loanIt->second.memberId() + " (due " +
                                        loanIt->second.dueDate().toString() + ").");
    }
    if (loanCountFor(mid) >= kMaxLoansPerMember) {
        return OperationResult::failure("Member " + mid + " already has " +
                                        std::to_string(kMaxLoansPerMember) +
                                        " active loans, which is the limit. Return a book first.");
    }

    const Date due = today.addDays(kLoanPeriodDays);
    loansByBook_.emplace(bid, Loan(bid, mid, due));
    return OperationResult::success("Issued " + quoted(bookIt->second.title()) + " (" + bid +
                                    ") to " + memberIt->second.name() + " (" + mid +
                                    "). Due " + due.toString() + ".");
}

OperationResult Library::returnBook(const std::string& bookId, const Date& today) {
    const std::string bid = validation::normalizeId(bookId);

    const auto bookIt = books_.find(bid);
    if (bookIt == books_.end()) {
        return OperationResult::failure("No book with ID " + bid + ".");
    }
    const auto loanIt = loansByBook_.find(bid);
    if (loanIt == loansByBook_.end()) {
        return OperationResult::failure("Book " + bid + " is not currently borrowed.");
    }

    const int daysLate = loanIt->second.daysOverdue(today);
    const std::string due = loanIt->second.dueDate().toString();
    const std::string title = bookIt->second.title();
    loansByBook_.erase(loanIt);

    std::string message = "Returned " + quoted(title) + " (" + bid + ").";
    if (daysLate > 0) {
        message += " It was " + std::to_string(daysLate) + " day(s) late (due " + due + ").";
    } else {
        message += " Returned on time.";
    }
    return OperationResult::success(message);
}

// -------------------------------------------------------------- import ----

ImportSummary Library::importMembers(const std::vector<Member>& incoming) {
    ImportSummary summary;
    for (const Member& member : incoming) {
        const std::string id = validation::normalizeId(member.id());
        const std::string name = textutil::normalizeSpaces(member.name());
        std::string error = validation::checkId(id, "Member ID");
        if (error.empty()) error = validation::checkText(name, "Name");
        if (!error.empty()) {
            ++summary.membersSkipped;
            summary.notes.push_back("Member " + member.id() + " skipped: " + error);
            continue;
        }
        if (members_.count(id) != 0) {
            ++summary.membersSkipped;
            summary.notes.push_back("Member " + id + " already exists; skipped.");
            continue;
        }
        members_.emplace(id, Member(id, name));
        ++summary.membersAdded;
    }
    return summary;
}

ImportSummary Library::importBooks(const std::vector<Book>& incoming,
                                   const std::vector<Loan>& incomingLoans) {
    ImportSummary summary;
    std::set<std::string> addedIds;  // books that really entered the library in this call

    for (const Book& book : incoming) {
        const std::string id = validation::normalizeId(book.id());
        const std::string title = textutil::normalizeSpaces(book.title());
        const std::string author = textutil::normalizeSpaces(book.author());
        const std::string category = textutil::normalizeSpaces(book.category());

        std::string error = validation::checkId(id, "Book ID");
        if (error.empty()) error = validation::checkText(title, "Title");
        if (error.empty()) error = validation::checkText(author, "Author");
        if (error.empty()) error = validation::checkText(category, "Category");
        if (error.empty()) error = validation::checkYear(book.year());
        if (!error.empty()) {
            ++summary.booksSkipped;
            summary.notes.push_back("Book " + book.id() + " skipped: " + error);
            continue;
        }
        if (books_.count(id) != 0) {
            ++summary.booksSkipped;
            summary.notes.push_back("Book " + id + " already exists; skipped (existing record kept).");
            continue;
        }
        books_.emplace(id, Book(id, title, author, category, book.year()));
        addedIds.insert(id);
        ++summary.booksAdded;
    }

    // A loan is only taken over when ITS book was added just now. (Existing books
    // keep their existing loan state.)
    for (const Loan& loan : incomingLoans) {
        const std::string bookId = validation::normalizeId(loan.bookId());
        const std::string memberId = validation::normalizeId(loan.memberId());
        if (addedIds.count(bookId) == 0 || loansByBook_.count(bookId) != 0) {
            continue;
        }
        if (members_.count(memberId) == 0) {
            // The data says this member has a book, but we have no record of the member.
            // Register a placeholder so the loan is not lost; the name can be fixed later.
            members_.emplace(memberId, Member(memberId, "Unknown member"));
            ++summary.placeholderMembers;
            summary.notes.push_back("Member " + memberId +
                                    " has a loan but was not in the member list; added a "
                                    "placeholder (use 'Update member name' to correct it).");
        }
        loansByBook_.emplace(bookId, Loan(bookId, memberId, loan.dueDate()));
        ++summary.loansAdded;
    }
    return summary;
}

// ----------------------------------------------------------- questions ----

bool Library::hasBook(const std::string& id) const {
    return books_.count(validation::normalizeId(id)) != 0;
}

bool Library::hasMember(const std::string& id) const {
    return members_.count(validation::normalizeId(id)) != 0;
}

std::optional<Book> Library::findBook(const std::string& id) const {
    const auto it = books_.find(validation::normalizeId(id));
    if (it == books_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<Member> Library::findMember(const std::string& id) const {
    const auto it = members_.find(validation::normalizeId(id));
    if (it == members_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<Loan> Library::findLoan(const std::string& bookId) const {
    const auto it = loansByBook_.find(validation::normalizeId(bookId));
    if (it == loansByBook_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool Library::isAvailable(const std::string& bookId) const {
    const std::string id = validation::normalizeId(bookId);
    return books_.count(id) != 0 && loansByBook_.count(id) == 0;
}

std::size_t Library::loanCountFor(const std::string& memberId) const {
    const std::string id = validation::normalizeId(memberId);
    return static_cast<std::size_t>(
        std::count_if(loansByBook_.begin(), loansByBook_.end(),
                      [&id](const std::pair<const std::string, Loan>& entry) {
                          return entry.second.memberId() == id;
                      }));
}

std::vector<Book> Library::books() const {
    std::vector<Book> result;
    result.reserve(books_.size());
    for (const auto& entry : books_) {
        result.push_back(entry.second);
    }
    return result;
}

std::vector<Member> Library::members() const {
    std::vector<Member> result;
    result.reserve(members_.size());
    for (const auto& entry : members_) {
        result.push_back(entry.second);
    }
    return result;
}

std::vector<Loan> Library::loans() const {
    std::vector<Loan> result;
    result.reserve(loansByBook_.size());
    for (const auto& entry : loansByBook_) {
        result.push_back(entry.second);
    }
    std::sort(result.begin(), result.end(), [](const Loan& a, const Loan& b) {
        if (a.dueDate() != b.dueDate()) {
            return a.dueDate() < b.dueDate();
        }
        return a.bookId() < b.bookId();
    });
    return result;
}

std::vector<Book> Library::searchBooks(const std::string& query) const {
    const std::string needle = textutil::normalizeSpaces(query);
    std::vector<Book> result;
    for (const auto& entry : books_) {
        const Book& book = entry.second;
        if (textutil::containsIgnoreCase(book.id(), needle) ||
            textutil::containsIgnoreCase(book.title(), needle) ||
            textutil::containsIgnoreCase(book.author(), needle) ||
            textutil::containsIgnoreCase(book.category(), needle) ||
            textutil::containsIgnoreCase(std::to_string(book.year()), needle)) {
            result.push_back(book);
        }
    }
    return result;
}
