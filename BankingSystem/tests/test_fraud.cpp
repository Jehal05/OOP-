#include "TestHarness.h"
#include "Fraud.h"
#include "Utils.h"

static Transaction tx(TxType t, double amt, int hour = 12) {
    time_t base = util::now();
    std::tm tmv{};
    localtime_r(&base, &tmv);
    tmv.tm_hour = hour; tmv.tm_min = 0; tmv.tm_sec = 0; tmv.tm_isdst = -1;
    return Transaction("R1", "SB1", t, Category::Other, amt, 0, mktime(&tmv));
}

int main() {
    FraudContext ctx;
    ctx.balanceBefore = 100000;

    LargeAmountRule large;
    CHECK(large.check(tx(TxType::Withdraw, 60000), ctx));                    // absolute threshold
    CHECK(!large.check(tx(TxType::Withdraw, 5000), ctx));
    CHECK(!large.check(tx(TxType::Deposit, 900000), ctx));                   // credits are never flagged
    FraudContext poor; poor.balanceBefore = 20000;
    CHECK(large.check(tx(TxType::Withdraw, 12000), poor));                   // > 50% of balance

    OddHourRule odd;
    CHECK(odd.check(tx(TxType::Withdraw, 100, 2), ctx));
    CHECK(!odd.check(tx(TxType::Withdraw, 100, 14), ctx));
    CHECK(!odd.check(tx(TxType::Deposit, 100, 2), ctx));

    RapidTransactionRule rapid;
    FraudContext busy;
    for (int i = 0; i < 3; ++i) busy.recent.push_back(tx(TxType::Withdraw, 10));
    CHECK(!rapid.check(tx(TxType::Withdraw, 10), busy));                     // 4th in window
    busy.recent.push_back(tx(TxType::Withdraw, 10));
    CHECK(rapid.check(tx(TxType::Withdraw, 10), busy));                      // 5th in window

    NewBeneficiaryHighValueRule nb;
    FraudContext np; np.newPayee = true;
    CHECK(nb.check(tx(TxType::TransferOut, 20000), np));
    CHECK(!nb.check(tx(TxType::TransferOut, 2000), np));
    CHECK(!nb.check(tx(TxType::TransferOut, 20000), ctx));

    RepeatedFailureRule rf;
    FraudContext fails; fails.failedOps = 3;
    CHECK(rf.check(tx(TxType::Withdraw, 10), fails));
    CHECK(!rf.check(tx(TxType::Withdraw, 10), ctx));

    // detector runs all registered rules polymorphically
    FraudDetector d;
    CHECK(d.ruleCount() == 5);
    auto hits = d.evaluate(tx(TxType::Withdraw, 80000, 3), ctx);             // large AND odd hour
    CHECK(hits.size() == 2);
    CHECK(d.evaluate(tx(TxType::Withdraw, 100, 12), ctx).empty());
    return finish("test_fraud");
}
