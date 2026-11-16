#include "TestHarness.h"
#include "Statement.h"
#include "Utils.h"
#include <fstream>
#include <sstream>

static std::vector<Transaction> sample() {
    time_t base = util::addDays(util::now(), -40);
    std::vector<Transaction> v;
    v.emplace_back("T1", "SB1", TxType::Deposit,  Category::Other, 10000, 10000, base);
    v.emplace_back("T2", "SB1", TxType::Withdraw, Category::Food,   1000,  9000, util::addDays(base, 1));
    v.emplace_back("T3", "SB1", TxType::Withdraw, Category::Rent,   5000,  4000, util::addDays(base, 2));
    v.emplace_back("T4", "SB1", TxType::Withdraw, Category::Food,    500,  3500, util::addDays(base, 3));
    v.emplace_back("T5", "SB1", TxType::Deposit,  Category::Other,  2500,  6000, util::addDays(base, 35));
    return v;
}

int main() {
    // summary
    Statement all("SB1", "Asha", sample());
    auto s = all.summary();
    CHECK(s.count == 5);
    CHECK_NEAR(s.credits, 12500, 1e-9);
    CHECK_NEAR(s.debits, 6500, 1e-9);
    CHECK_NEAR(s.net, 6000, 1e-9);
    CHECK_NEAR(s.largest, 10000, 1e-9);
    CHECK_NEAR(s.average, 19000.0 / 5, 1e-9);

    // filters
    StatementFilter f; f.type = TxType::Withdraw;
    CHECK(Statement("SB1", "A", sample()).filter(f).rows().size() == 3);
    StatementFilter g; g.category = Category::Food;
    CHECK(Statement("SB1", "A", sample()).filter(g).rows().size() == 2);
    StatementFilter h; h.minAmount = 1000; h.maxAmount = 5000;
    CHECK(Statement("SB1", "A", sample()).filter(h).rows().size() == 3);       // 1000, 5000, 2500
    StatementFilter d; d.from = util::addDays(util::now(), -39); d.to = util::addDays(util::now(), -38);
    CHECK(Statement("SB1", "A", sample()).filter(d).rows().size() == 2);
    CHECK(StatementFilter{}.empty() && !f.empty());

    // sorting
    Statement byAmt("SB1", "A", sample());
    byAmt.sort(SortKey::Amount, false);
    CHECK(byAmt.rows().front().refId() == "T1" && byAmt.rows().back().refId() == "T4");
    byAmt.sort(SortKey::Date, false);
    CHECK(byAmt.rows().front().refId() == "T5");
    byAmt.sort(SortKey::Date, true);
    CHECK(byAmt.rows().front().refId() == "T1");

    // monthly grouping
    CHECK(all.byMonth().size() >= 2);
    size_t total = 0;
    for (const auto& [m, rows] : all.byMonth()) { (void)m; total += rows.size(); }
    CHECK(total == 5);

    // export
    std::string dir = tempDir("stmt");
    std::string path = all.exportText(dir, f, 6000);
    std::ifstream in(path);
    std::stringstream ss; ss << in.rdbuf();
    std::string text = ss.str();
    CHECK(text.find("NOVA BANK - ACCOUNT STATEMENT") != std::string::npos);
    CHECK(text.find("TXN") == std::string::npos);                               // sample refs are T1..T5
    CHECK(text.find("T3") != std::string::npos);
    CHECK(text.find("Net change") != std::string::npos);
    std::filesystem::remove_all(dir);

    // spending analyzer: only categorised spending, sorted, percentages sum to 100
    auto slices = SpendingAnalyzer::analyze(sample(), "");
    CHECK(slices.size() == 2 && slices[0].category == Category::Rent);
    CHECK_NEAR(slices[0].total, 5000, 1e-9);
    CHECK_NEAR(slices[1].total, 1500, 1e-9);
    CHECK_NEAR(slices[0].percent + slices[1].percent, 100.0, 1e-9);
    CHECK(SpendingAnalyzer::analyze(sample(), "1999-01").empty());
    return finish("test_statement");
}
