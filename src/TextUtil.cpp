#include "TextUtil.h"

#include <algorithm>
#include <cctype>

namespace textutil {

bool isSpace(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

std::string trim(const std::string& text) {
    std::size_t first = 0;
    while (first < text.size() && isSpace(text[first])) {
        ++first;
    }
    std::size_t last = text.size();
    while (last > first && isSpace(text[last - 1])) {
        --last;
    }
    return text.substr(first, last - first);
}

std::string normalizeSpaces(const std::string& text) {
    std::string result;
    bool pendingSpace = false;
    for (char c : text) {
        if (isSpace(c)) {
            pendingSpace = !result.empty();
        } else {
            if (pendingSpace) {
                result += ' ';
            }
            pendingSpace = false;
            result += c;
        }
    }
    return result;
}

std::string toLower(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

std::string toUpper(const std::string& text) {
    std::string result = text;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

bool containsIgnoreCase(const std::string& haystack, const std::string& needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool hasControlCharacter(const std::string& text) {
    return std::any_of(text.begin(), text.end(), [](char c) {
        return std::iscntrl(static_cast<unsigned char>(c)) != 0;
    });
}

bool parseInteger(const std::string& text, int& value) {
    if (text.empty() || text.size() > 9) {
        return false;
    }
    int result = 0;
    for (char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        result = result * 10 + (c - '0');
    }
    value = result;
    return true;
}

std::string truncate(const std::string& text, std::size_t maxBytes) {
    if (text.size() <= maxBytes) {
        return text;
    }
    if (maxBytes <= 3) {
        return text.substr(0, maxBytes);
    }
    std::size_t cut = maxBytes - 3;
    // Step back while we are at a UTF-8 continuation byte (10xxxxxx).
    while (cut > 0 && (static_cast<unsigned char>(text[cut]) & 0xC0U) == 0x80U) {
        --cut;
    }
    return text.substr(0, cut) + "...";
}

std::string padRight(const std::string& text, std::size_t width) {
    return text.size() >= width ? text : text + std::string(width - text.size(), ' ');
}

}  // namespace textutil
