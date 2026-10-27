// TextUtil: small, reusable helpers for working with text.
// (Shared by every layer; contains no library-specific logic.)
#pragma once

#include <cstddef>
#include <string>

namespace textutil {

bool isSpace(char c);

// Removes leading and trailing whitespace.
std::string trim(const std::string& text);

// Trims and collapses every run of whitespace into a single space.
std::string normalizeSpaces(const std::string& text);

std::string toLower(const std::string& text);
std::string toUpper(const std::string& text);

// Case-insensitive "does haystack contain needle?" (an empty needle matches).
bool containsIgnoreCase(const std::string& haystack, const std::string& needle);

bool hasControlCharacter(const std::string& text);

// Parses a non-negative whole number of at most 9 digits ("2008"). Returns false
// for anything else (letters, signs, decimals, empty text, too many digits).
bool parseInteger(const std::string& text, int& value);

// Shortens text to at most maxBytes bytes, ending with "..." when it was cut.
// Never cuts in the middle of a UTF-8 character.
std::string truncate(const std::string& text, std::size_t maxBytes);

std::string padRight(const std::string& text, std::size_t width);

}  // namespace textutil
