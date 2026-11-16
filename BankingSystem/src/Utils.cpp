#include "Utils.h"
#include <cstdio>
#include <cstdint>
#include <cmath>
#include <random>
#include <algorithm>
#include <cctype>

namespace util {

std::string number(double v, int decimals) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", decimals, std::fabs(v));
    std::string s = buf, frac;
    auto dot = s.find('.');
    if (dot != std::string::npos) { frac = s.substr(dot); s = s.substr(0, dot); }
    std::string out;
    if (s.size() <= 3) out = s;
    else {                                   // last 3 digits, then groups of 2
        out = s.substr(s.size() - 3);
        s.erase(s.size() - 3);
        while (!s.empty()) {
            size_t take = std::min<size_t>(2, s.size());
            out = s.substr(s.size() - take) + "," + out;
            s.erase(s.size() - take);
        }
    }
    bool neg = v < 0 && std::fabs(v) >= 0.005;
    return (neg ? "-" : "") + out + frac;
}

std::string money(double v) {
    std::string n = number(v);
    if (!n.empty() && n[0] == '-') return "-\xE2\x82\xB9" + n.substr(1);   // -₹...
    return "\xE2\x82\xB9" + n;
}

std::string fixedWidth(double v) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%015.2f", v);
    return buf;
}

static std::tm toTm(time_t t) {
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    return tmv;
}
static std::string fmt(time_t t, const char* f) {
    std::tm tmv = toTm(t);
    char buf[64];
    std::strftime(buf, sizeof buf, f, &tmv);
    return buf;
}

time_t now() { return std::time(nullptr); }
std::string fmtDateTime(time_t t) { return fmt(t, "%Y-%m-%d %H:%M:%S"); }
std::string fmtDate(time_t t)     { return fmt(t, "%Y-%m-%d"); }
std::string fmtShort(time_t t)    { return fmt(t, "%d %b %H:%M"); }
std::string monthKey(time_t t)    { return fmt(t, "%Y-%m"); }
std::string compactDate(time_t t) { return fmt(t, "%Y%m%d"); }
int hourOf(time_t t)              { return toTm(t).tm_hour; }

bool parseDateTime(const std::string& s, time_t& out) {
    int y, mo, d, h, mi, se;
    if (std::sscanf(s.c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &se) != 6) return false;
    std::tm tmv{};
    tmv.tm_year = y - 1900; tmv.tm_mon = mo - 1; tmv.tm_mday = d;
    tmv.tm_hour = h; tmv.tm_min = mi; tmv.tm_sec = se; tmv.tm_isdst = -1;
    out = std::mktime(&tmv);
    return out != (time_t)-1;
}

bool parseDate(const std::string& s, time_t& out) {
    int y, mo, d;
    char extra;
    if (std::sscanf(s.c_str(), "%d-%d-%d%c", &y, &mo, &d, &extra) != 3) return false;
    if (y < 1970 || mo < 1 || mo > 12 || d < 1 || d > 31) return false;
    std::tm tmv{};
    tmv.tm_year = y - 1900; tmv.tm_mon = mo - 1; tmv.tm_mday = d; tmv.tm_hour = 12; tmv.tm_isdst = -1;
    std::tm check = tmv;
    out = std::mktime(&tmv);
    return out != (time_t)-1 && tmv.tm_mday == check.tm_mday;   // rejects 2026-02-31
}

time_t addMonths(time_t t, int months) {
    std::tm tmv = toTm(t);
    tmv.tm_mon += months; tmv.tm_isdst = -1;
    return std::mktime(&tmv);
}
time_t addDays(time_t t, int days) { return t + (time_t)days * 86400; }

int monthsBetween(time_t from, time_t to) {
    std::tm a = toTm(from), b = toTm(to);
    int m = (b.tm_year - a.tm_year) * 12 + (b.tm_mon - a.tm_mon);
    if (b.tm_mday < a.tm_mday) --m;
    return m;
}

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace((unsigned char)s[b])) ++b;
    while (e > b && std::isspace((unsigned char)s[e - 1])) --e;
    return s.substr(b, e - b);
}
std::string upper(std::string s) { for (auto& c : s) c = (char)std::toupper((unsigned char)c); return s; }
std::string lower(std::string s) { for (auto& c : s) c = (char)std::tolower((unsigned char)c); return s; }

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) { if (c == sep) { out.push_back(cur); cur.clear(); } else if (c != '\r') cur += c; }
    out.push_back(cur);
    return out;
}
std::string join(const std::vector<std::string>& p, char sep) {
    std::string r;
    for (size_t i = 0; i < p.size(); ++i) { if (i) r += sep; r += p[i]; }
    return r;
}
std::string sanitize(const std::string& s) {
    std::string r = trim(s);
    for (auto& c : r) if (c == ',' || c == '\n' || c == '\r' || c == '|') c = ' ';
    return r;
}
bool isDigits(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return std::isdigit(c); });
}
std::string maskAccount(const std::string& a) {
    if (a.size() <= 4) return a;
    return "XXXX-" + a.substr(a.size() - 4);
}

std::string randomSalt() {
    static std::mt19937_64 rng{std::random_device{}()};
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", (unsigned long long)rng());
    return buf;
}

// Iterated FNV-1a over salt+pin. Good enough to demonstrate salted hashing in a
// student project; a real bank would use bcrypt/argon2.
std::string hashPin(const std::string& pin, const std::string& salt) {
    std::uint64_t h = 1469598103934665603ULL;
    const std::string data = salt + ":" + pin;
    for (int round = 0; round < 2000; ++round)
        for (unsigned char c : data) { h ^= c; h *= 1099511628211ULL; h ^= (h >> 29); }
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", (unsigned long long)h);
    return buf;
}

} // namespace util
