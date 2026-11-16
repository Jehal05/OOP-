// Statement.h - Smart Statement (filters, sorting, summary, monthly grouping, .txt export)
//               and SpendingAnalyzer (category totals for the bar chart).
#pragma once
#include <vector>
#include <map>
#include <optional>
#include <string>
#include "Models.h"

struct StatementFilter {
    std::optional<time_t> from, to;
    std::optional<TxType> type;
    std::optional<double> minAmount, maxAmount;
    std::optional<Category> category;
    bool matches(const Transaction& t) const;
    bool empty() const { return !from && !to && !type && !minAmount && !maxAmount && !category; }
    std::string describe() const;
};

enum class SortKey { Date, Amount };

struct StatementSummary {
    double credits = 0, debits = 0, net = 0, average = 0, largest = 0;
    size_t count = 0;
};

class Statement {
    std::string accNo_, holder_;
    std::vector<Transaction> rows_;
public:
    Statement(std::string accNo, std::string holder, std::vector<Transaction> all);
    Statement& filter(const StatementFilter& f);                 // keeps matching rows (chainable)
    Statement& sort(SortKey key, bool ascending);
    const std::vector<Transaction>& rows() const { return rows_; }
    StatementSummary summary() const;
    std::map<std::string, std::vector<Transaction>> byMonth() const;       // "2026-11" -> rows
    // Writes a bank-style plain-text statement; returns the path written.
    std::string exportText(const std::string& dir, const StatementFilter& f, double currentBalance) const;
};

class SpendingAnalyzer {
public:
    struct Slice { Category category; double total; double percent; };
    // Only categorised spending (withdrawals and transfers out) is counted. month = "YYYY-MM" or "" for all time.
    static std::vector<Slice> analyze(const std::vector<Transaction>& txs, const std::string& month);
};
