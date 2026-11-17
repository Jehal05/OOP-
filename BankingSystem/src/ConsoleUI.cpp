#include "ConsoleUI.h"
#include "Exceptions.h"
#include "Utils.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
  #include <conio.h>
  #include <io.h>
  #define ISATTY _isatty
  #define FILENO _fileno
#else
  #include <unistd.h>
  #include <termios.h>
  #define ISATTY isatty
  #define FILENO fileno
#endif

namespace ui {

void init() {}

// ===================== text helpers =====================
static std::string rep(char ch, size_t n) { return std::string(n, ch); }

std::string padRight(const std::string& s, size_t w) { return s.size() >= w ? s : s + std::string(w - s.size(), ' '); }
std::string padLeft(const std::string& s, size_t w)  { return s.size() >= w ? s : std::string(w - s.size(), ' ') + s; }
std::string truncate(const std::string& s, size_t w) {
    if (s.size() <= w || w < 2) return s;
    return s.substr(0, w - 1) + "~";
}

// ===================== layout =====================
void blank() { std::cout << "\n"; }
void line(const std::string& s) { std::cout << s << "\n"; }

void screen(const std::string& breadcrumb) {
    std::cout << "\n" << rep('=', 78) << "\n NOVA BANK - Banking Management System\n" << rep('=', 78) << "\n"
              << " " << breadcrumb << "\n\n";
}

void title(const std::string& text) {
    std::cout << " " << text << "\n " << rep('-', std::min<size_t>(text.size(), 76)) << "\n";
}

void box(const std::string& heading, const std::vector<std::string>& lines, int width) {
    if (!heading.empty()) title(heading);
    else std::cout << " " << rep('-', (size_t)width - 2) << "\n";
    for (const auto& l : lines) std::cout << "   " << l << "\n";
    std::cout << "\n";
}

void table(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows,
           const std::vector<Align>& aligns, size_t maxCol) {
    size_t n = headers.size();
    std::vector<size_t> w(n);
    for (size_t i = 0; i < n; ++i) w[i] = headers[i].size();
    for (const auto& r : rows)
        for (size_t i = 0; i < n && i < r.size(); ++i) w[i] = std::max(w[i], std::min(r[i].size(), maxCol));
    auto cell = [&](const std::string& s, size_t i) {
        std::string t = truncate(s, w[i]);
        return (i < aligns.size() && aligns[i] == Align::Right) ? padLeft(t, w[i]) : padRight(t, w[i]);
    };
    std::string head = " ", dash = " ";
    for (size_t i = 0; i < n; ++i) { head += cell(headers[i], i) + "  "; dash += rep('-', w[i]) + "  "; }
    auto trimEnd = [](std::string t) { while (!t.empty() && t.back() == ' ') t.pop_back(); return t; };
    std::cout << trimEnd(head) << "\n" << trimEnd(dash) << "\n";
    for (const auto& r : rows) {
        std::string l = " ";
        for (size_t i = 0; i < n; ++i) l += cell(i < r.size() ? r[i] : "", i) + "  ";
        std::cout << trimEnd(l) << "\n";
    }
}

void banner(Kind kind, const std::string& msg) {
    const char* label = "INFO";
    switch (kind) {
        case Kind::Success: label = "SUCCESS"; break;
        case Kind::Error:   label = "ERROR";   break;
        case Kind::Warning: label = "WARNING"; break;
        case Kind::Info:    label = "INFO";    break;
    }
    std::cout << "\n [" << label << "] " << msg << "\n";
}

std::string percent(double f) {
    f = std::max(0.0, std::min(1.0, f));
    return std::to_string((int)std::lround(f * 100)) + "%";
}

void list(const std::vector<std::string>& labels, const std::vector<std::string>& valueText) {
    size_t lw = 0, vw = 0;
    for (const auto& l : labels) lw = std::max(lw, l.size());
    for (const auto& v : valueText) vw = std::max(vw, v.size());
    for (size_t i = 0; i < labels.size(); ++i) std::cout << "  " << padRight(labels[i], lw) << "  " << padLeft(valueText[i], vw) << "\n";
}

// ===================== input =====================
static bool stdinIsTty() { return ISATTY(FILENO(stdin)) != 0; }

std::string readLine(const std::string& prompt) {
    std::cout << " > " << prompt << " ";
    std::cout.flush();
    std::string s;
    if (!std::getline(std::cin, s)) throw InputClosed{};
    if (!stdinIsTty()) std::cout << util::trim(s) << "\n";   // echo like a terminal would (scripted runs)
    return util::trim(s);
}

int readInt(const std::string& prompt, int mn, int mx, std::optional<int> def) {
    std::string p = prompt + (def ? " [" + std::to_string(*def) + "]" : "");
    for (;;) {
        std::string s = readLine(p);
        if (s.empty() && def) return *def;
        char* end = nullptr;
        long v = std::strtol(s.c_str(), &end, 10);
        if (!s.empty() && *end == '\0' && v >= mn && v <= mx) return (int)v;
        banner(Kind::Error, "Please enter a whole number between " + std::to_string(mn) + " and " + std::to_string(mx) + ".");
    }
}

static bool parseAmount(const std::string& raw, double& out) {
    std::string s;
    for (char ch : raw) if (ch != ',') s += ch;                 // allow 1,00,000
    if (s.empty()) return false;
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (*end != '\0' || !std::isfinite(v)) return false;
    auto dot = s.find('.');
    if (dot != std::string::npos && s.size() - dot - 1 > 2) return false;   // max 2 decimals
    out = v;
    return true;
}

double readAmount(const std::string& prompt) {
    for (;;) {
        double v;
        if (parseAmount(readLine(prompt), v) && v > 0 && v <= 1e9) return v;
        banner(Kind::Error, "Enter a positive amount with at most 2 decimals (max 100 crore).");
    }
}

std::optional<double> readOptionalAmount(const std::string& prompt, const std::string& hint) {
    for (;;) {
        std::string s = readLine(prompt + " " + hint);
        if (s.empty()) return std::nullopt;
        double v;
        if (parseAmount(s, v) && v >= 0 && v <= 1e9) return v;
        banner(Kind::Error, "Enter a valid amount or press Enter to skip.");
    }
}

std::string readPin(const std::string& prompt) {
    std::cout << " > " << prompt << " ";
    std::cout.flush();
    std::string pin;
    if (!stdinIsTty()) {                                        // scripted/redirected input: read a line, show it masked
        if (!std::getline(std::cin, pin)) throw InputClosed{};
        for (size_t i = 0; i < pin.size(); ++i) std::cout << "*";
        std::cout << "\n";
        return pin;
    }
#ifdef _WIN32
    for (int ch; (ch = _getch()) != '\r' && ch != '\n';) {
        if (ch == 8 || ch == 127) { if (!pin.empty()) { pin.pop_back(); std::cout << "\b \b"; } }
        else if (ch >= 32 && ch < 127) { pin += (char)ch; std::cout << "*"; }
    }
#else
    termios oldt{}, newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    for (int ch; (ch = std::getchar()) != '\n' && ch != EOF;) {
        if (ch == 127 || ch == 8) { if (!pin.empty()) { pin.pop_back(); std::cout << "\b \b"; std::cout.flush(); } }
        else if (ch >= 32 && ch < 127) { pin += (char)ch; std::cout << "*"; std::cout.flush(); }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    std::cout << "\n";
    return pin;
}

bool confirm(const std::string& prompt, bool def) {
    for (;;) {
        std::string s = util::lower(readLine(prompt + (def ? " [Y/n]" : " [y/N]")));
        if (s.empty()) return def;
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
        banner(Kind::Error, "Please answer y or n.");
    }
}

void pause() {
    std::cout << "\n Press Enter to continue...";
    std::cout.flush();
    std::string s;
    if (!std::getline(std::cin, s)) throw InputClosed{};
    if (!stdinIsTty()) std::cout << "\n";
}

} // namespace ui
