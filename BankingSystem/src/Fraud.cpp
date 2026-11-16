#include "Fraud.h"
#include "Utils.h"

bool LargeAmountRule::check(const Transaction& tx, const FraudContext& ctx) const {
    if (tx.credit()) return false;
    return tx.amount() >= 50000.0 || (ctx.balanceBefore > 0 && tx.amount() > 0.5 * ctx.balanceBefore && tx.amount() >= 10000.0);
}
std::string LargeAmountRule::message(const Transaction& tx) const {
    return "Large debit of " + util::money(tx.amount()) + " (" + toString(tx.type()) + ")";
}

bool RapidTransactionRule::check(const Transaction& tx, const FraudContext& ctx) const {
    (void)tx;
    return ctx.recent.size() + 1 >= 5;                           // 4 earlier + this one within 10 minutes
}
std::string RapidTransactionRule::message(const Transaction&) const {
    return "5 or more transactions within 10 minutes";
}

bool NewBeneficiaryHighValueRule::check(const Transaction& tx, const FraudContext& ctx) const {
    return ctx.newPayee && tx.type() == TxType::TransferOut && tx.amount() >= 10000.0;
}
std::string NewBeneficiaryHighValueRule::message(const Transaction& tx) const {
    return "High-value transfer of " + util::money(tx.amount()) + " to a payee added in the last 24 hours";
}

bool OddHourRule::check(const Transaction& tx, const FraudContext&) const {
    int h = util::hourOf(tx.timestamp());
    return !tx.credit() && h >= 1 && h < 4;
}
std::string OddHourRule::message(const Transaction& tx) const {
    return "Debit of " + util::money(tx.amount()) + " at an unusual hour (" + util::fmtShort(tx.timestamp()) + ")";
}

bool RepeatedFailureRule::check(const Transaction&, const FraudContext& ctx) const { return ctx.failedOps >= 3; }
std::string RepeatedFailureRule::message(const Transaction&) const {
    return "Transaction succeeded after 3 or more failed attempts";
}

FraudDetector::FraudDetector() {
    addRule(std::make_unique<LargeAmountRule>());
    addRule(std::make_unique<RapidTransactionRule>());
    addRule(std::make_unique<NewBeneficiaryHighValueRule>());
    addRule(std::make_unique<OddHourRule>());
    addRule(std::make_unique<RepeatedFailureRule>());
}

std::vector<std::pair<std::string, std::string>> FraudDetector::evaluate(const Transaction& tx, const FraudContext& ctx) const {
    std::vector<std::pair<std::string, std::string>> hits;
    for (const auto& r : rules_)
        if (r->check(tx, ctx)) hits.emplace_back(r->name(), r->message(tx));
    return hits;
}
