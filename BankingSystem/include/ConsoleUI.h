// ConsoleUI.h - ALL terminal presentation lives here (colors, boxes, tables, prompts).
// Business-logic classes never format output themselves.
#pragma once
#include <string>
#include <vector>
#include <optional>

namespace ui {

enum class Align { Left, Right, Center };
enum class Kind { Success, Error, Warning, Info };

// ---- setup / style ----
void init(int argc, char** argv);      // parses --ascii / --no-color / --color, NO_COLOR env, enables Windows VT
bool colorEnabled();
bool unicodeEnabled();

// ---- colors (return "" when colors are disabled) ----
namespace c {
std::string reset(); std::string bold(); std::string dim();
std::string red(); std::string green(); std::string yellow();
std::string blue(); std::string magenta(); std::string cyan();
}
std::string paint(const std::string& color, const std::string& text);

// ---- text measuring / padding (ANSI + UTF-8 aware) ----
size_t visibleWidth(const std::string& s);
std::string padRight(const std::string& s, size_t w);
std::string padLeft(const std::string& s, size_t w);
std::string center(const std::string& s, size_t w);
std::string truncate(const std::string& s, size_t w);

// ---- layout ----
void clear();
void screen(const std::string& breadcrumb);          // clear + header banner + breadcrumb
void footer(const std::string& hint);
void title(const std::string& text);                 // cyan section heading with underline
void box(const std::string& heading, const std::vector<std::string>& lines, int width = 78);
void table(const std::vector<std::string>& headers,
           const std::vector<std::vector<std::string>>& rows,
           const std::vector<Align>& aligns = {}, size_t maxColWidth = 30);
void banner(Kind kind, const std::string& message);
void blank();
void line(const std::string& s = "");                // println helper

// ---- visual widgets ----
std::string progressBar(double fraction, int width = 20);                    // [██████░░░░] 60%
std::string barString(double fraction, int width);                           // ██████
void barChart(const std::vector<std::string>& labels, const std::vector<double>& values,
              const std::vector<std::string>& valueText, int width = 28, int highlight = -1);

// ---- input (validating, never crashes on bad input) ----
std::string readLine(const std::string& prompt);
int readInt(const std::string& prompt, int min, int max, std::optional<int> def = std::nullopt);
double readAmount(const std::string& prompt);
std::optional<double> readOptionalAmount(const std::string& prompt, const std::string& hint = "(Enter to skip)");   // blank = skip
std::string readPin(const std::string& prompt);                              // masked on a real terminal
bool confirm(const std::string& prompt, bool def = false);
void pause();

} // namespace ui
