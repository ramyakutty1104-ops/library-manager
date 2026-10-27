#include "Validation.h"

#include <cctype>

#include "TextUtil.h"

namespace validation {

std::string normalizeId(const std::string& raw) {
    return textutil::toUpper(textutil::normalizeSpaces(raw));
}

std::string checkId(const std::string& id, const std::string& label) {
    if (id.empty()) {
        return label + " cannot be empty.";
    }
    if (id.size() > kMaxIdLength) {
        return label + " is too long (maximum " + std::to_string(kMaxIdLength) + " characters).";
    }
    for (char c : id) {
        const bool allowed = std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '_';
        if (!allowed) {
            return label + " may only contain letters, digits, '-' and '_' (no spaces).";
        }
    }
    return "";
}

std::string checkText(const std::string& text, const std::string& label) {
    if (text.empty()) {
        return label + " cannot be empty.";
    }
    if (text.size() > kMaxTextLength) {
        return label + " is too long (maximum " + std::to_string(kMaxTextLength) + " characters).";
    }
    if (textutil::hasControlCharacter(text)) {
        return label + " contains invalid control characters.";
    }
    return "";
}

std::string checkYear(int year) {
    if (year < kMinYear || year > kMaxYear) {
        return "Year must be between " + std::to_string(kMinYear) + " and " +
               std::to_string(kMaxYear) + ".";
    }
    return "";
}

}  // namespace validation
