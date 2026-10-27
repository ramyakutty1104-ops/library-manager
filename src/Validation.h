// Validation: the rules that decide whether a piece of text is acceptable
// as an ID, a name/title, or a year.
//
// Every check returns an error message, or an EMPTY string when the value is
// fine. The same rules are used when the user types something, when the
// Library receives it, and when a CSV row is read, so bad data is refused the
// same way everywhere.
#pragma once

#include <cstddef>
#include <string>

namespace validation {

constexpr std::size_t kMaxIdLength = 12;
constexpr std::size_t kMaxTextLength = 100;
constexpr int kMinYear = 1000;
constexpr int kMaxYear = 2100;

// Trims, collapses spaces and upper-cases, so "b001" and " B001 " mean "B001".
std::string normalizeId(const std::string& raw);

// `id` should already be normalised. `label` is used in the message ("Book ID").
std::string checkId(const std::string& id, const std::string& label);

// `text` should already have its spaces normalised.
std::string checkText(const std::string& text, const std::string& label);

std::string checkYear(int year);

}  // namespace validation
