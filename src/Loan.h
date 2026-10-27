// Loan: the fact that one member currently has one book, until a due date.
//
// Responsibility: link a book to a member by their IDs and answer
// "is this loan late?". A Loan stores IDs, not pointers, so it can never
// dangle if the collections it refers to change.
#pragma once

#include <string>

#include "Date.h"

class Loan {
public:
    Loan(std::string bookId, std::string memberId, Date dueDate);

    const std::string& bookId() const { return bookId_; }
    const std::string& memberId() const { return memberId_; }
    const Date& dueDate() const { return dueDate_; }

    // A book is late only AFTER its due date: on the due date itself it is not overdue.
    bool isOverdue(const Date& today) const;
    int daysOverdue(const Date& today) const;  // 0 when not overdue

private:
    std::string bookId_;
    std::string memberId_;
    Date dueDate_;
};
