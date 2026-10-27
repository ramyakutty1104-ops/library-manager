#include "Book.h"

#include <utility>

Book::Book(std::string id, std::string title, std::string author, std::string category, int year)
    : id_(std::move(id)),
      title_(std::move(title)),
      author_(std::move(author)),
      category_(std::move(category)),
      year_(year) {}

void Book::setTitle(std::string title) {
    title_ = std::move(title);
}

void Book::setAuthor(std::string author) {
    author_ = std::move(author);
}

void Book::setCategory(std::string category) {
    category_ = std::move(category);
}

void Book::setYear(int year) {
    year_ = year;
}
