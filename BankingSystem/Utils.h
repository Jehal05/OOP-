// Small helper functions used everywhere (text, time, money, PIN hashing).
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

namespace util {

inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

inline std::string upper(std::string s) {
    for (char& c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

inline std::string lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// Commas and new lines would break our CSV files, so replace them.
inline std::string clean(std::string s) {
    for (char& c : s) if (c == ',' || c == '\n' || c == '\r') c = ';';
    return trim(s);
}

inline std::vector<std::string> split(const std::string& line, char sep = ',') {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : line) {
        if (c == sep) { parts.push_back(cur); cur.clear(); }
        else cur += c;
    }
    parts.push_back(cur);
    return parts;
}

inline double round2(double v) { return std::round(v * 100.0) / 100.0; }

inline std::string money(double v) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "Rs %.2f", v);
    return buf;
}

inline std::string formatTime(time_t t) {
    char buf[32];
    std::tm* tmv = std::localtime(&t);
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", tmv);
    return buf;
}

inline std::string formatDate(time_t t) {
    char buf[32];
    std::tm* tmv = std::localtime(&t);
    std::strftime(buf, sizeof buf, "%Y-%m-%d", tmv);
    return buf;
}

inline std::string compactDate(time_t t) {          // 20261116
    char buf[16];
    std::tm* tmv = std::localtime(&t);
    std::strftime(buf, sizeof buf, "%Y%m%d", tmv);
    return buf;
}

inline int hourOf(time_t t) { return std::localtime(&t)->tm_hour; }

// PINs are never stored as plain text. This salted, repeated FNV-1a hash is fine for a learning
// project; a real bank would use bcrypt or argon2.
inline std::string hashPin(const std::string& pin, const std::string& salt) {
    unsigned long long h = 1469598103934665603ULL;
    std::string data = salt + ":" + pin;
    for (int round = 0; round < 1000; ++round)
        for (unsigned char c : data) { h ^= c; h *= 1099511628211ULL; }
    char buf[32];
    std::snprintf(buf, sizeof buf, "%016llx", h);
    return buf;
}

} // namespace util
