#include "Loan.h"

#include <utility>

Loan::Loan(std::string bookId, std::string memberId, Date dueDate)
    : bookId_(std::move(bookId)), memberId_(std::move(memberId)), dueDate_(dueDate) {}

bool Loan::isOverdue(const Date& today) const {
    return today > dueDate_;
}

int Loan::daysOverdue(const Date& today) const {
    return isOverdue(today) ? dueDate_.daysUntil(today) : 0;
}
