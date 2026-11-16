// Fraud.h - polymorphic fraud-detection rules.
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include "Models.h"

// Everything a rule may need to judge a transaction.
struct FraudContext {
    std::vector<Transaction> recent;     // this account's transactions in the last 10 minutes (excluding the new one)
    double balanceBefore = 0;            // balance before the transaction
    bool newPayee = false;               // payee was added less than 24h ago
    int failedOps = 0;                   // consecutive failed withdrawals/transfers in this session
};

class FraudRule {
public:
    virtual ~FraudRule() = default;
    virtual bool check(const Transaction& tx, const FraudContext& ctx) const = 0;
    virtual std::string name() const = 0;
    virtual std::string message(const Transaction& tx) const = 0;
};

class LargeAmountRule : public FraudRule {
public:
    bool check(const Transaction& tx, const FraudContext& ctx) const override;
    std::string name() const override { return "LargeAmount"; }
    std::string message(const Transaction& tx) const override;
};
class RapidTransactionRule : public FraudRule {
public:
    bool check(const Transaction& tx, const FraudContext& ctx) const override;
    std::string name() const override { return "RapidTransactions"; }
    std::string message(const Transaction& tx) const override;
};
class NewBeneficiaryHighValueRule : public FraudRule {
public:
    bool check(const Transaction& tx, const FraudContext& ctx) const override;
    std::string name() const override { return "NewPayeeHighValue"; }
    std::string message(const Transaction& tx) const override;
};
class OddHourRule : public FraudRule {
public:
    bool check(const Transaction& tx, const FraudContext& ctx) const override;
    std::string name() const override { return "OddHour"; }
    std::string message(const Transaction& tx) const override;
};
class RepeatedFailureRule : public FraudRule {
public:
    bool check(const Transaction& tx, const FraudContext& ctx) const override;
    std::string name() const override { return "RepeatedFailures"; }
    std::string message(const Transaction& tx) const override;
};

class FraudDetector {
    std::vector<std::unique_ptr<FraudRule>> rules_;
public:
    FraudDetector();                                              // registers the default rules
    void addRule(std::unique_ptr<FraudRule> r) { rules_.push_back(std::move(r)); }
    size_t ruleCount() const { return rules_.size(); }
    // Returns (ruleName, message) for every rule that fires.
    std::vector<std::pair<std::string, std::string>> evaluate(const Transaction& tx, const FraudContext& ctx) const;
};
