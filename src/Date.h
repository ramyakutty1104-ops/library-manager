// Date: a calendar date (year-month-day) with strict validation and day arithmetic.
//
// Internally a Date is a single number: days since 1970-01-01. That makes
// "add 14 days", "how many days late?" and comparisons trivial and exact, and
// it handles leap years and month/year boundaries correctly.
#pragma once

#include <optional>
#include <string>

class Date {
public:
    // Creates a date, or returns std::nullopt if it does not exist
    // (for example 2023-02-29 or 2024-13-01). Years 1000-9999 are accepted.
    static std::optional<Date> fromYmd(int y, int m, int d);

    // Parses strictly formatted text "YYYY-MM-DD" (exactly 10 characters).
    static std::optional<Date> parse(const std::string& text);

    // Today's date according to the computer's clock (local time).
    static Date today();

    static bool isLeapYear(int y);
    static int daysInMonth(int y, int m);

    int year() const;
    int month() const;
    int day() const;

    std::string toString() const;  // "YYYY-MM-DD"

    Date addDays(int days) const;
    int daysUntil(const Date& later) const;  // later - this (negative if earlier)

    bool operator==(const Date& other) const { return serial_ == other.serial_; }
    bool operator!=(const Date& other) const { return serial_ != other.serial_; }
    bool operator<(const Date& other) const { return serial_ < other.serial_; }
    bool operator>(const Date& other) const { return serial_ > other.serial_; }
    bool operator<=(const Date& other) const { return serial_ <= other.serial_; }
    bool operator>=(const Date& other) const { return serial_ >= other.serial_; }

private:
    explicit Date(int serialDays) : serial_(serialDays) {}

    int serial_;  // days since 1970-01-01
};
