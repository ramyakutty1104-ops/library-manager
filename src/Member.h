// Member: a person who may borrow books.
//
// Responsibility: hold a member's ID (permanent) and name (can be corrected).
#pragma once

#include <string>

class Member {
public:
    Member(std::string id, std::string name);

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }

    void setName(std::string name);

private:
    std::string id_;
    std::string name_;
};
