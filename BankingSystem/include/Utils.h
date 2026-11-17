// Utils.h - formatting, time, string and hashing helpers shared by all modules.
#pragma once
#include <string>
#include <vector>
#include <ctime>

namespace util {

// ---- money formatting (Indian digit grouping, e.g. 12,34,567.50) ----
std::string number(double v, int decimals = 2);
std::string money(double v);                 // "Rs 12,34,567.50"
std::string moneyShort(double v);            // "Rs 12,34,568" (no paise, for compact tables)
std::string fixedWidth(double v);            // zero padded, constant length (for in-place CSV patching)

// ---- time helpers ----
time_t now();
std::string fmtDateTime(time_t t);           // 2026-11-16 23:10:04
std::string fmtDate(time_t t);               // 2026-11-16
std::string fmtShort(time_t t);              // 16 Nov 23:10
std::string monthKey(time_t t);              // 2026-11
std::string compactDate(time_t t);           // 20261116
int hourOf(time_t t);
bool parseDateTime(const std::string& s, time_t& out);
bool parseDate(const std::string& s, time_t& out);   // YYYY-MM-DD
time_t addMonths(time_t t, int months);
time_t addDays(time_t t, int days);
int monthsBetween(time_t from, time_t to);   // whole months, can be negative

// ---- strings ----
std::string trim(const std::string& s);
std::string upper(std::string s);
std::string lower(std::string s);
std::vector<std::string> split(const std::string& s, char sep);
std::string join(const std::vector<std::string>& parts, char sep);
std::string sanitize(const std::string& s);   // removes commas/newlines/pipes (names, nicknames)
std::string cleanText(const std::string& s);  // trims and drops only newlines (free text kept intact)
std::string csvEscape(const std::string& s);  // ',' '\n' '\r' '%' -> %2C %0A %0D %25 so free text survives CSV
std::string csvUnescape(const std::string& s);
bool isDigits(const std::string& s);
std::string maskAccount(const std::string& accNo);   // XXXX-1001

// ---- security (educational salted hash, NOT production grade) ----
std::string randomSalt();
std::string hashPin(const std::string& pin, const std::string& salt);

} // namespace util
