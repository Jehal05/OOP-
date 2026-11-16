#include "ConsoleUI.h"
#include "Exceptions.h"
#include "Utils.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>

#ifdef _WIN32
  #include <windows.h>
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

// ===================== style state =====================
static bool g_color = true;
static bool g_unicode = true;
static bool g_formFeed = false;   // BANK_SCREEN_MARK=1 -> emit \f instead of clearing (used to capture screenshots)

bool colorEnabled() { return g_color; }
bool unicodeEnabled() { return g_unicode; }

void init(int argc, char** argv) {
    g_color = ISATTY(FILENO(stdout)) != 0;
    if (std::getenv("NO_COLOR")) g_color = false;
    if (std::getenv("FORCE_COLOR")) g_color = true;
    if (std::getenv("BANK_ASCII")) g_unicode = false;
    if (std::getenv("BANK_SCREEN_MARK")) g_formFeed = true;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--ascii")) g_unicode = false;
        else if (!std::strcmp(argv[i], "--no-color")) g_color = false;
        else if (!std::strcmp(argv[i], "--color")) g_color = true;
    }
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (h != INVALID_HANDLE_VALUE && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004 /*ENABLE_VIRTUAL_TERMINAL_PROCESSING*/);
#endif
}

namespace c {
static std::string code(const char* s) { return g_color ? s : ""; }
std::string reset()   { return code("\x1b[0m"); }
std::string bold()    { return code("\x1b[1m"); }
std::string dim()     { return code("\x1b[90m"); }
std::string red()     { return code("\x1b[31m"); }
std::string green()   { return code("\x1b[32m"); }
std::string yellow()  { return code("\x1b[33m"); }
std::string blue()    { return code("\x1b[34m"); }
std::string magenta() { return code("\x1b[35m"); }
std::string cyan()    { return code("\x1b[36m"); }
}
std::string paint(const std::string& color, const std::string& text) {
    if (!g_color || color.empty()) return text;
    return color + text + c::reset();
}

// ===================== glyph sets =====================
struct Glyphs {
    const char *tl, *tr, *bl, *br, *h, *v, *lt, *rt, *tt, *bt, *x;
    const char *dtl, *dtr, *dbl, *dbr, *dh, *dv;     // heavy/double frame for the banner
    const char *full, *empty, *bullet, *ok, *err, *warn, *info, *crumb, *star;
};
static const Glyphs& G() {
    static const Glyphs uni{"┌","┐","└","┘","─","│","├","┤","┬","┴","┼",
                            "╔","╗","╚","╝","═","║",
                            "█","░","▸","✔","✖","⚠","ℹ","›","◆"};
    static const Glyphs asc{"+","+","+","+","-","|","+","+","+","+","+",
                            "+","+","+","+","=","|",
                            "#",".",">","[OK]","[X]","[!]","[i]",">","*"};
    return g_unicode ? uni : asc;
}
static std::string rep(const std::string& s, size_t n) { std::string r; for (size_t i = 0; i < n; ++i) r += s; return r; }

// ===================== measuring =====================
size_t visibleWidth(const std::string& s) {
    size_t w = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = (unsigned char)s[i];
        if (ch == 0x1b && i + 1 < s.size() && s[i + 1] == '[') {      // skip ANSI sequence
            i += 2;
            while (i < s.size() && !std::isalpha((unsigned char)s[i])) ++i;
            continue;
        }
        if ((ch & 0xC0) != 0x80) ++w;                                 // count UTF-8 lead bytes only
    }
    return w;
}
std::string padRight(const std::string& s, size_t w) { size_t v = visibleWidth(s); return v >= w ? s : s + std::string(w - v, ' '); }
std::string padLeft(const std::string& s, size_t w)  { size_t v = visibleWidth(s); return v >= w ? s : std::string(w - v, ' ') + s; }
std::string center(const std::string& s, size_t w) {
    size_t v = visibleWidth(s);
    if (v >= w) return s;
    size_t left = (w - v) / 2;
    return std::string(left, ' ') + s + std::string(w - v - left, ' ');
}
std::string truncate(const std::string& s, size_t w) {
    if (visibleWidth(s) <= w || s.find('\x1b') != std::string::npos) return s;
    std::string out; size_t count = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = (unsigned char)s[i];
        if ((ch & 0xC0) != 0x80) { if (count == w - 1) break; ++count; }
        out += s[i];
    }
    // make sure we do not cut inside a multi-byte char
    return out + (g_unicode ? "…" : "~");
}

// ===================== layout =====================
void blank() { std::cout << "\n"; }
void line(const std::string& s) { std::cout << s << "\n"; }

void clear() {
    if (g_formFeed) { std::cout << "\f"; return; }
    if (!ISATTY(FILENO(stdout))) { std::cout << "\n"; return; }
    std::cout << "\x1b[2J\x1b[H";
}

void screen(const std::string& breadcrumb) {
    clear();
    const Glyphs& g = G();
    const size_t W = 78;
    std::string name = std::string(" ") + g.star + "  NOVA BANK  " + g.bullet + "  Banking Management System";
    std::cout << c::cyan() << g.dtl << rep(g.dh, W) << g.dtr << c::reset() << "\n";
    std::cout << c::cyan() << g.dv << c::reset() << c::bold() << padRight(name, W) << c::reset()
              << c::cyan() << g.dv << c::reset() << "\n";
    std::cout << c::cyan() << g.dbl << rep(g.dh, W) << g.dbr << c::reset() << "\n";
    std::string out = breadcrumb;
    // replace the generic " > " separators with the style's crumb glyph
    std::string sep = " > ", rep2 = std::string(" ") + g.crumb + " ";
    size_t pos = 0;
    while ((pos = out.find(sep, pos)) != std::string::npos) { out.replace(pos, sep.size(), rep2); pos += rep2.size(); }
    std::cout << c::dim() << " " << out << c::reset() << "\n\n";
}

void footer(const std::string& hint) {
    std::cout << "\n" << c::dim() << rep(G().h, 78) << "\n " << hint << c::reset() << "\n";
}

void title(const std::string& text) {
    std::cout << c::bold() << c::cyan() << " " << text << c::reset() << "\n"
              << c::dim() << " " << rep(G().h, std::min<size_t>(visibleWidth(text), 76)) << c::reset() << "\n";
}

void box(const std::string& heading, const std::vector<std::string>& lines, int width) {
    const Glyphs& g = G();
    size_t inner = (size_t)width - 2;
    std::cout << g.tl;
    if (!heading.empty()) {
        std::string h = std::string(" ") + heading + " ";
        std::cout << c::bold() << c::cyan() << h << c::reset() << rep(g.h, inner - visibleWidth(h));
    } else std::cout << rep(g.h, inner);
    std::cout << g.tr << "\n";
    for (const auto& l : lines)
        std::cout << g.v << " " << padRight(l, inner - 2) << " " << g.v << "\n";
    std::cout << g.bl << rep(g.h, inner) << g.br << "\n";
}

void table(const std::vector<std::string>& headers, const std::vector<std::vector<std::string>>& rows,
           const std::vector<Align>& aligns, size_t maxCol) {
    const Glyphs& g = G();
    size_t n = headers.size();
    std::vector<size_t> w(n);
    for (size_t i = 0; i < n; ++i) w[i] = visibleWidth(headers[i]);
    for (const auto& r : rows)
        for (size_t i = 0; i < n && i < r.size(); ++i) w[i] = std::max(w[i], std::min(visibleWidth(r[i]), maxCol));
    auto hline = [&](const char* l, const char* m, const char* r) {
        std::cout << l;
        for (size_t i = 0; i < n; ++i) std::cout << rep(g.h, w[i] + 2) << (i + 1 < n ? m : r);
        std::cout << "\n";
    };
    auto cell = [&](const std::string& s, size_t i) {
        std::string t = truncate(s, w[i]);
        Align a = i < aligns.size() ? aligns[i] : Align::Left;
        if (a == Align::Right) return padLeft(t, w[i]);
        if (a == Align::Center) return center(t, w[i]);
        return padRight(t, w[i]);
    };
    hline(g.tl, g.tt, g.tr);
    std::cout << g.v;
    for (size_t i = 0; i < n; ++i) std::cout << " " << c::bold() << c::cyan() << cell(headers[i], i) << c::reset() << " " << g.v;
    std::cout << "\n";
    hline(g.lt, g.x, g.rt);
    for (const auto& r : rows) {
        std::cout << g.v;
        for (size_t i = 0; i < n; ++i) std::cout << " " << cell(i < r.size() ? r[i] : "", i) << " " << g.v;
        std::cout << "\n";
    }
    hline(g.bl, g.bt, g.br);
}

void banner(Kind kind, const std::string& msg) {
    const Glyphs& g = G();
    std::string color, icon, label;
    switch (kind) {
        case Kind::Success: color = c::green();  icon = g.ok;   label = "SUCCESS"; break;
        case Kind::Error:   color = c::red();    icon = g.err;  label = "ERROR";   break;
        case Kind::Warning: color = c::yellow(); icon = g.warn; label = "WARNING"; break;
        case Kind::Info:    color = c::blue();   icon = g.info; label = "INFO";    break;
    }
    const std::string head = std::string(" ") + icon + " " + label + "  ";
    const size_t indent = visibleWidth(head), room = 78 > indent ? 78 - indent : 40;
    // word-wrap the message so the banner never runs past 80 columns
    std::vector<std::string> lines;
    std::string cur, word;
    auto flush = [&]() {
        if (word.empty()) return;
        if (!cur.empty() && cur.size() + 1 + word.size() > room) { lines.push_back(cur); cur.clear(); }
        cur += (cur.empty() ? "" : " ") + word;
        word.clear();
    };
    for (char ch : msg) { if (ch == ' ') flush(); else word += ch; }
    flush();
    if (!cur.empty()) lines.push_back(cur);
    std::cout << "\n";
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i == 0) std::cout << color << c::bold() << head << c::reset() << color << lines[i] << c::reset() << "\n";
        else std::cout << std::string(indent, ' ') << color << lines[i] << c::reset() << "\n";
    }
}

// ===================== widgets =====================
std::string barString(double f, int width) {
    f = std::max(0.0, std::min(1.0, f));
    int filled = (int)std::lround(f * width);
    return rep(G().full, (size_t)filled) + rep(G().empty, (size_t)(width - filled));
}
std::string progressBar(double f, int width) {
    f = std::max(0.0, std::min(1.0, f));
    char pct[16]; std::snprintf(pct, sizeof pct, " %3d%%", (int)std::lround(f * 100));
    std::string col = f >= 1.0 ? c::green() : (f >= 0.5 ? c::cyan() : c::yellow());
    return "[" + paint(col, barString(f, width)) + "]" + pct;
}
void barChart(const std::vector<std::string>& labels, const std::vector<double>& values,
              const std::vector<std::string>& valueText, int width, int highlight) {
    double mx = 0; for (double v : values) mx = std::max(mx, v);
    size_t lw = 0; for (const auto& l : labels) lw = std::max(lw, visibleWidth(l));
    for (size_t i = 0; i < labels.size(); ++i) {
        double f = mx > 0 ? values[i] / mx : 0;
        std::string col = (int)i == highlight ? c::yellow() : c::cyan();
        std::cout << "  " << padRight(labels[i], lw) << " " << paint(col, barString(f, width)) << "  " << valueText[i]
                  << ((int)i == highlight ? paint(c::yellow(), std::string("  ") + G().warn + " top") : "") << "\n";
    }
}

// ===================== input =====================
static bool stdinIsTty() { return ISATTY(FILENO(stdin)) != 0; }

std::string readLine(const std::string& prompt) {
    std::cout << " " << c::cyan() << G().bullet << c::reset() << " " << prompt << " ";
    std::cout.flush();
    std::string s;
    if (!std::getline(std::cin, s)) throw InputClosed{};
    if (!stdinIsTty()) std::cout << c::bold() << util::trim(s) << c::reset() << "\n";   // echo like a terminal would (scripted runs)
    return util::trim(s);
}

int readInt(const std::string& prompt, int mn, int mx, std::optional<int> def) {
    std::string p = prompt + (def ? c::dim() + " [" + std::to_string(*def) + "]" + c::reset() : "");
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
        std::string s = readLine(prompt + c::dim() + " " + hint + c::reset());
        if (s.empty()) return std::nullopt;
        double v;
        if (parseAmount(s, v) && v >= 0 && v <= 1e9) return v;
        banner(Kind::Error, "Enter a valid amount or press Enter to skip.");
    }
}

std::string readPin(const std::string& prompt) {
    std::cout << " " << c::cyan() << G().bullet << c::reset() << " " << prompt << " ";
    std::cout.flush();
    std::string pin;
    if (!stdinIsTty()) {                                        // scripted/redirected input: read a line, show it masked
        if (!std::getline(std::cin, pin)) throw InputClosed{};
        for (size_t i = 0; i < pin.size(); ++i) std::cout << (g_unicode ? "\xE2\x80\xA2" : "*");
        std::cout << "\n";
        return pin;
    }
#ifdef _WIN32
    for (int ch; (ch = _getch()) != '\r' && ch != '\n';) {
        if (ch == 8 || ch == 127) { if (!pin.empty()) { pin.pop_back(); std::cout << "\b \b"; } }
        else if (ch >= 32 && ch < 127) { pin += (char)ch; std::cout << (g_unicode ? "\xE2\x80\xA2" : "*"); }
    }
#else
    termios oldt{}, newt{};
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    for (int ch; (ch = std::getchar()) != '\n' && ch != EOF;) {
        if (ch == 127 || ch == 8) { if (!pin.empty()) { pin.pop_back(); std::cout << "\b \b"; std::cout.flush(); } }
        else if (ch >= 32 && ch < 127) { pin += (char)ch; std::cout << (g_unicode ? "\xE2\x80\xA2" : "*"); std::cout.flush(); }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    std::cout << "\n";
    return pin;
}

bool confirm(const std::string& prompt, bool def) {
    for (;;) {
        std::string s = util::lower(readLine(prompt + c::dim() + (def ? " [Y/n]" : " [y/N]") + c::reset()));
        if (s.empty()) return def;
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
        banner(Kind::Error, "Please answer y or n.");
    }
}

void pause() {
    std::cout << "\n " << c::dim() << "Press Enter to continue" << (g_unicode ? "…" : "...") << c::reset();
    std::cout.flush();
    std::string s;
    if (!std::getline(std::cin, s)) throw InputClosed{};
    if (!stdinIsTty()) std::cout << "\n";
}

} // namespace ui
