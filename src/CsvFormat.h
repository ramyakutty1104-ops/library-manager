// CsvFormat: reading and writing ONE line of CSV text.
//
// Handles the standard rules: fields are separated by commas; a field that
// contains a comma or a quote is wrapped in "double quotes", and a quote
// inside it is written twice ("").
#pragma once

#include <string>
#include <vector>

namespace csv {

// Splits one line into fields. Returns false and sets `error` when the line is
// malformed (for example an unterminated quote).
bool parseLine(const std::string& line, std::vector<std::string>& fields, std::string& error);

// Wraps the field in quotes only when it needs them.
std::string escapeField(const std::string& field);

// Escapes every field and joins them with commas (no trailing newline).
std::string joinLine(const std::vector<std::string>& fields);

}  // namespace csv
