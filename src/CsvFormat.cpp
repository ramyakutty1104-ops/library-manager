#include "CsvFormat.h"

namespace csv {

bool parseLine(const std::string& line, std::vector<std::string>& fields, std::string& error) {
    fields.clear();
    error.clear();

    std::string current;
    bool inQuotes = false;      // currently inside "..."
    bool fieldStarted = false;  // something (or an opening quote) was seen in this field
    bool quoteClosed = false;   // the quoted part of this field has ended

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    current += '"';  // "" inside quotes means one literal quote
                    ++i;
                } else {
                    inQuotes = false;
                    quoteClosed = true;
                }
            } else {
                current += c;
            }
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
            fieldStarted = false;
            quoteClosed = false;
        } else if (c == '"') {
            if (fieldStarted) {
                error = "unexpected quote character inside a field";
                return false;
            }
            inQuotes = true;
            fieldStarted = true;
        } else {
            if (quoteClosed) {
                error = "unexpected text after a closing quote";
                return false;
            }
            current += c;
            fieldStarted = true;
        }
    }

    if (inQuotes) {
        error = "unterminated quoted field";
        return false;
    }
    fields.push_back(current);
    return true;
}

std::string escapeField(const std::string& field) {
    const bool needsQuotes = field.find_first_of(",\"\r\n") != std::string::npos ||
                             (!field.empty() && (field.front() == ' ' || field.back() == ' '));
    if (!needsQuotes) {
        return field;
    }
    std::string result = "\"";
    for (char c : field) {
        if (c == '"') {
            result += '"';  // double the quote
        }
        result += c;
    }
    result += '"';
    return result;
}

std::string joinLine(const std::vector<std::string>& fields) {
    std::string result;
    for (std::size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) {
            result += ',';
        }
        result += escapeField(fields[i]);
    }
    return result;
}

}  // namespace csv
