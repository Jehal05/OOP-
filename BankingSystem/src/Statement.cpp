#include "Statement.h"
#include "Utils.h"
#include "Exceptions.h"
#include <algorithm>
#include <numeric>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>

bool StatementFilter::matches(const Transaction& t) const {
    if (from && t.timestamp() < *from) return false;
    if (to && t.timestamp() > *to + 86399) return false;          // 'to' is inclusive of the whole day
    if (type && t.type() != *type) return false;
    if (minAmount && t.amount() < *minAmount) return false;
    if (maxAmount && t.amount() > *maxAmount) return false;
    if (category && t.category() != *category) return false;
    return true;
}

std::string StatementFilter::describe() const {
    std::vector<std::string> p;
    if (from) p.push_back("from " + util::fmtDate(*from));
    if (to) p.push_back("to " + util::fmtDate(*to));
    if (type) p.push_back("type = " + toString(*type));
    if (category) p.push_back("category = " + toString(*category));
    if (minAmount) p.push_back("min " + util::money(*minAmount));
    if (maxAmount) p.push_back("max " + util::money(*maxAmount));
    if (p.empty()) return "none (all transactions)";
    std::string r;
    for (size_t i = 0; i < p.size(); ++i) r += (i ? ", " : "") + p[i];
    return r;
}

Statement::Statement(std::string accNo, std::string holder, std::vector<Transaction> all)
    : accNo_(std::move(accNo)), holder_(std::move(holder)), rows_(std::move(all)) {
    std::sort(rows_.begin(), rows_.end());                        // uses Transaction::operator<
}

Statement& Statement::filter(const StatementFilter& f) {
    rows_.erase(std::remove_if(rows_.begin(), rows_.end(), [&](const Transaction& t) { return !f.matches(t); }), rows_.end());
    return *this;
}

Statement& Statement::sort(SortKey key, bool ascending) {
    std::stable_sort(rows_.begin(), rows_.end(), [&](const Transaction& a, const Transaction& b) {
        bool less = key == SortKey::Date ? a < b : a.amount() < b.amount();
        bool greater = key == SortKey::Date ? b < a : b.amount() < a.amount();
        return ascending ? less : greater;
    });
    return *this;
}

StatementSummary Statement::summary() const {
    StatementSummary s;
    s.count = rows_.size();
    s.credits = std::accumulate(rows_.begin(), rows_.end(), 0.0, [](double a, const Transaction& t) { return a + (t.credit() ? t.amount() : 0.0); });
    s.debits  = std::accumulate(rows_.begin(), rows_.end(), 0.0, [](double a, const Transaction& t) { return a + (t.credit() ? 0.0 : t.amount()); });
    s.net = s.credits - s.debits;
    if (!rows_.empty()) {
        s.average = (s.credits + s.debits) / (double)rows_.size();
        s.largest = std::max_element(rows_.begin(), rows_.end(), [](const Transaction& a, const Transaction& b) { return a.amount() < b.amount(); })->amount();
    }
    return s;
}

std::map<std::string, std::vector<Transaction>> Statement::byMonth() const {
    std::map<std::string, std::vector<Transaction>> m;
    for (const auto& t : rows_) m[util::monthKey(t.timestamp())].push_back(t);
    return m;
}

std::string Statement::exportText(const std::string& dir, const StatementFilter& f, double currentBalance) const {
    std::filesystem::create_directories(dir);
    std::string path = dir + "/" + accNo_ + "_" + util::compactDate(util::now()) + ".txt";
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) throw FileIOException("cannot write " + path);
    const std::string bar(86, '=');
    out << bar << "\n"
        << "                              NOVA BANK - ACCOUNT STATEMENT\n"
        << bar << "\n"
        << "Account Number : " << accNo_ << "\n"
        << "Account Holder : " << holder_ << "\n"
        << "Generated On   : " << util::fmtDateTime(util::now()) << "\n"
        << "Filters        : " << f.describe() << "\n"
        << "Current Balance: " << util::money(currentBalance) << "\n"
        << std::string(86, '-') << "\n"
        << std::left << std::setw(20) << "Date" << std::setw(21) << "Reference" << std::setw(14) << "Type"
        << std::setw(11) << "Category" << std::right << std::setw(15) << "Amount" << std::setw(16) << "Balance" << "\n"
        << std::string(86, '-') << "\n";
    for (const auto& t : rows_) {
        out << std::left << std::setw(20) << util::fmtDateTime(t.timestamp()) << std::setw(21) << t.refId()
            << std::setw(14) << toString(t.type()) << std::setw(11) << (t.credit() ? "-" : toString(t.category()))
            << std::right << std::setw(15) << ((t.credit() ? "+" : "-") + util::number(t.amount()))
            << std::setw(16) << util::number(t.balanceAfter()) << "\n";
    }
    StatementSummary s = summary();
    out << std::string(86, '-') << "\n"
        << "Transactions : " << s.count << "\n"
        << "Total credits: " << util::money(s.credits) << "\n"
        << "Total debits : " << util::money(s.debits) << "\n"
        << "Net change   : " << (s.net >= 0 ? "+" : "") << util::money(s.net) << "\n"
        << "Average txn  : " << util::money(s.average) << "\n"
        << "Largest txn  : " << util::money(s.largest) << "\n"
        << bar << "\n"
        << "This is a computer generated statement and does not require a signature.\n";
    return path;
}

std::vector<SpendingAnalyzer::Slice> SpendingAnalyzer::analyze(const std::vector<Transaction>& txs, const std::string& month) {
    std::map<Category, double> totals;
    double all = 0;
    for (const auto& t : txs) {
        if (t.type() != TxType::Withdraw && t.type() != TxType::TransferOut) continue;
        if (!month.empty() && util::monthKey(t.timestamp()) != month) continue;
        totals[t.category()] += t.amount();
        all += t.amount();
    }
    std::vector<Slice> out;
    for (const auto& [cat, total] : totals) out.push_back({cat, total, all > 0 ? total * 100.0 / all : 0.0});
    std::sort(out.begin(), out.end(), [](const Slice& a, const Slice& b) { return a.total > b.total; });
    return out;
}
