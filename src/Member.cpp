#include "Member.h"

#include <utility>

Member::Member(std::string id, std::string name) : id_(std::move(id)), name_(std::move(name)) {}

void Member::setName(std::string name) {
    name_ = std::move(name);
}
