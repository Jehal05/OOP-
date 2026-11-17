#pragma once
// Plain-text console helpers: headings, simple tables, messages and validated input.
// No colors, no box drawing, no charts - everything is ordinary ASCII text.
#include <string>
#include <vector>
#include <optional>

namespace ui {

enum class Align { Left, Right };
enum class Kind { Success, Error, Warning, Info };

void init();                                          // nothing to set up on most systems (kept for symmetry)

std::string padRight(const std::string& s, size_t w);
std::string padLeft(const std::string& s, size_t w);
std::string truncate(const std::string& s, size_t w);

void screen(const std::string& breadcrumb);           // header line + where you are (e.g. "Home > Customer > Statement")
void title(const std::string& text);                  // section heading with an underline
void box(const std::string& heading, const std::vector<std::string>& lines, int width = 78);   // heading + indented lines
void table(const std::vector<std::string>& headers,
           const std::vector<std::vector<std::string>>& rows,
           const std::vector<Align>& aligns = {}, size_t maxColWidth = 30);
void banner(Kind kind, const std::string& message);   // [SUCCESS] / [ERROR] / [WARNING] / [INFO] line
void blank();
void line(const std::string& s = "");

std::string percent(double fraction);                 // 0.6 -> "60%"
void list(const std::vector<std::string>& labels, const std::vector<std::string>& valueText);  // aligned "label  value" lines

std::string readLine(const std::string& prompt);
int readInt(const std::string& prompt, int min, int max, std::optional<int> def = std::nullopt);
double readAmount(const std::string& prompt);
std::optional<double> readOptionalAmount(const std::string& prompt, const std::string& hint = "(Enter to skip)");   // blank = skip
std::string readPin(const std::string& prompt);       // typed characters show as * on a real terminal
bool confirm(const std::string& prompt, bool def = false);
void pause();

} // namespace ui
