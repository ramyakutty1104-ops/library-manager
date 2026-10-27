#include "Date.h"

#include <cstdio>
#include <ctime>

namespace {

// Converts a calendar date to "days since 1970-01-01" (proleptic Gregorian
// calendar). Algorithm by Howard Hinnant (public domain).
int daysFromCivil(int y, int m, int d) {
    y -= (m <= 2) ? 1 : 0;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const int yearOfEra = y - era * 400;
    const int monthIndex = (m > 2) ? m - 3 : m + 9;  // March = 0 ... February = 11
    const int dayOfYear = (153 * monthIndex + 2) / 5 + d - 1;
    const int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return era * 146097 + dayOfEra - 719468;
}

// The reverse of daysFromCivil.
void civilFromDays(int z, int& y, int& m, int& d) {
    z += 719468;
    const int era = (z >= 0 ? z : z - 146096) / 146097;
    const int dayOfEra = z - era * 146097;
    const int yearOfEra =
        (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    const int dayOfYear = dayOfEra - (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const int monthIndex = (5 * dayOfYear + 2) / 153;
    d = dayOfYear - (153 * monthIndex + 2) / 5 + 1;
    m = (monthIndex < 10) ? monthIndex + 3 : monthIndex - 9;
    y = yearOfEra + era * 400 + ((m <= 2) ? 1 : 0);
}

}  // namespace

bool Date::isLeapYear(int y) {
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

int Date::daysInMonth(int y, int m) {
    static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) {
        return 0;
    }
    return (m == 2 && isLeapYear(y)) ? 29 : kDays[m - 1];
}

std::optional<Date> Date::fromYmd(int y, int m, int d) {
    if (y < 1000 || y > 9999 || m < 1 || m > 12 || d < 1 || d > daysInMonth(y, m)) {
        return std::nullopt;
    }
    return Date(daysFromCivil(y, m, d));
}

std::optional<Date> Date::parse(const std::string& text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') {
        return std::nullopt;
    }
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (text[i] < '0' || text[i] > '9') {
            return std::nullopt;
        }
    }
    const int y = std::stoi(text.substr(0, 4));  // safe: digits only, short
    const int m = std::stoi(text.substr(5, 2));
    const int d = std::stoi(text.substr(8, 2));
    return fromYmd(y, m, d);
}

Date Date::today() {
    const std::time_t now = std::time(nullptr);
    const std::tm* local = std::localtime(&now);
    if (local != nullptr) {
        const std::optional<Date> result =
            fromYmd(local->tm_year + 1900, local->tm_mon + 1, local->tm_mday);
        if (result) {
            return *result;
        }
    }
    return Date(0);  // 1970-01-01, only if the clock is unusable
}

int Date::year() const {
    int y = 0, m = 0, d = 0;
    civilFromDays(serial_, y, m, d);
    return y;
}

int Date::month() const {
    int y = 0, m = 0, d = 0;
    civilFromDays(serial_, y, m, d);
    return m;
}

int Date::day() const {
    int y = 0, m = 0, d = 0;
    civilFromDays(serial_, y, m, d);
    return d;
}

std::string Date::toString() const {
    int y = 0, m = 0, d = 0;
    civilFromDays(serial_, y, m, d);
    char buffer[64];
    std::snprintf(buffer, sizeof buffer, "%04d-%02d-%02d", y, m, d);
    return buffer;
}

Date Date::addDays(int days) const {
    return Date(serial_ + days);
}

int Date::daysUntil(const Date& later) const {
    return later.serial_ - serial_;
}
