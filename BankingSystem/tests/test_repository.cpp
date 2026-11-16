#include "TestHarness.h"
#include "Repository.h"
#include "Account.h"
#include "Models.h"
#include "Utils.h"
#include <fstream>

int main() {
    std::string dir = tempDir("repo");
    time_t now = util::now();
    Repository<Account> repo(dir + "/accounts.csv");
    repo.add(std::make_unique<SavingsAccount>("SB1", "Asha", 5000, now));
    repo.add(std::make_unique<CurrentAccount>("CA1", "Ravi", 100, now));
    repo.saveAll();

    Repository<Account> loaded(dir + "/accounts.csv");
    loaded.load();
    CHECK(loaded.size() == 2);
    CHECK(loaded.find("SB1") && loaded.find("SB1")->holder() == "Asha");
    CHECK(loaded.find("NOPE") == nullptr);

    // in-place patch (seekp): file size stays identical because balances are fixed width
    auto sizeBefore = std::filesystem::file_size(dir + "/accounts.csv");
    loaded.find("SB1")->deposit(2500);
    CHECK(loaded.patch("SB1"));
    CHECK(std::filesystem::file_size(dir + "/accounts.csv") == sizeBefore);
    Repository<Account> again(dir + "/accounts.csv");
    again.load();
    CHECK_NEAR(again.find("SB1")->balance(), 7500, 1e-9);
    CHECK_NEAR(again.find("CA1")->balance(), 100, 1e-9);          // neighbour record untouched

    // name change alters the length -> falls back to a full rewrite, still correct
    again.find("CA1")->setHolder("Ravi Kumar Senior");
    CHECK(again.patch("CA1"));
    Repository<Account> third(dir + "/accounts.csv");
    third.load();
    CHECK(third.find("CA1")->holder() == "Ravi Kumar Senior");
    CHECK(third.size() == 2);

    // append-only transaction log
    Repository<Transaction> txs(dir + "/tx.csv");
    txs.addAndAppend(std::make_unique<Transaction>("T1", "SB1", TxType::Deposit, Category::Other, 10, 10, now, "a"));
    txs.addAndAppend(std::make_unique<Transaction>("T2", "SB1", TxType::Withdraw, Category::Food, 5, 5, now, "b"));
    Repository<Transaction> txLoaded(dir + "/tx.csv");
    txLoaded.load();
    CHECK(txLoaded.size() == 2 && txLoaded.find("T2")->category() == Category::Food);

    // remove, where(), and corrupt lines are skipped instead of crashing
    CHECK(txLoaded.remove("T1") && txLoaded.size() == 1);
    CHECK(txLoaded.where([](const Transaction& t) { return t.amount() > 100; }).empty());
    { std::ofstream o(dir + "/tx.csv", std::ios::app); o << "garbage line\n"; }
    Repository<Transaction> bad(dir + "/tx.csv");
    bad.load();
    CHECK(bad.size() == 2 && bad.skippedLines() == 1);

    // free text with commas, percent signs and newlines survives a CSV round trip (regression: "₹9,800" lost its comma)
    {
        Repository<Transaction> r(dir + "/notes.csv");
        r.addAndAppend(std::make_unique<Transaction>("N1", "SB1", TxType::Deposit, Category::Other, 1, 1, now, "Debit of \xE2\x82\xB9" "9,800.00, 100% sure"));
        Repository<Transaction> back(dir + "/notes.csv");
        back.load();
        CHECK(back.size() == 1 && back.find("N1")->note() == "Debit of \xE2\x82\xB9" "9,800.00, 100% sure");
        Alert al("A1", "SB1", "N1", "Rule", "Large debit of 1,00,000", now);
        auto ab = Alert::fromCsv(al.toCsv());
        CHECK(ab && ab->message() == "Large debit of 1,00,000");
    }

    // move-only ownership (Rule of 5)
    Repository<Account> moved = std::move(third);
    CHECK(moved.size() == 2);
    std::filesystem::remove_all(dir);
    return finish("test_repository");
}
