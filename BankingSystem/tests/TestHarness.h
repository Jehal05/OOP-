// Tiny dependency-free test harness (CHECK / CHECK_THROWS / CHECK_NEAR).
#pragma once
#include <iostream>
#include <string>
#include <cmath>
#include <filesystem>
#include <chrono>

static int g_checks = 0, g_failed = 0;

#define CHECK(cond) do { ++g_checks; if (!(cond)) { ++g_failed; \
    std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #cond "\n"; } } while (0)
#define CHECK_NEAR(a, b, eps) do { ++g_checks; if (std::fabs((a) - (b)) > (eps)) { ++g_failed; \
    std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  " #a " (" << (a) << ") != " #b " (" << (b) << ")\n"; } } while (0)
#define CHECK_THROWS(expr, ExType) do { ++g_checks; bool caught_ = false; \
    try { expr; } catch (const ExType&) { caught_ = true; } catch (...) {} \
    if (!caught_) { ++g_failed; std::cerr << "  FAIL " << __FILE__ << ":" << __LINE__ << "  expected " #ExType " from " #expr "\n"; } } while (0)

inline std::string tempDir(const std::string& tag) {
    auto p = std::filesystem::temp_directory_path() /
             ("novabank_" + tag + "_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(p);
    return p.string();
}

inline int finish(const char* suite) {
    std::cout << suite << ": " << (g_checks - g_failed) << "/" << g_checks << " checks passed\n";
    return g_failed == 0 ? 0 : 1;
}
