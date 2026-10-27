// Book: the catalogue record of one book.
//
// Responsibility: hold and protect a book's descriptive details (ID, title,
// author, category, year). It deliberately does NOT know whether the book is
// borrowed - that fact lives in exactly one place, the Loan (see Library).
// Keeping a single source of truth means "status" can never disagree with
// the loans.
#pragma once

#include <string>

class Book {
public:
    Book(std::string id, std::string title, std::string author, std::string category, int year);

    const std::string& id() const { return id_; }
    const std::string& title() const { return title_; }
    const std::string& author() const { return author_; }
    const std::string& category() const { return category_; }
    int year() const { return year_; }

    // The ID never changes after creation; everything else can be corrected.
    void setTitle(std::string title);
    void setAuthor(std::string author);
    void setCategory(std::string category);
    void setYear(int year);

private:
    std::string id_;
    std::string title_;
    std::string author_;
    std::string category_;
    int year_;
};
