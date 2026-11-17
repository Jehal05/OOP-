// Fraud detection. Every rule is a small class derived from FraudRule; the bank holds a list of rules
// and asks each one (runtime polymorphism) whether a new debit looks suspicious.
#pragma once
#include <ctime>
#include <string>
#include <vector>
#include "Transaction.h"

struct Alert {
    std::string id, accNo, rule, message;
    time_t time;
    bool reviewed;      // an admin has looked at it
    bool seen;          // the customer has seen it

    std::string toCsv() const;
    static Alert fromCsv(const std::string& line);
};

class FraudRule {
public:
    virtual ~FraudRule() = default;
    virtual std::string name() const = 0;
    // returns true (and fills message) if the new transaction looks suspicious
    virtual bool check(const Transaction& tx, const std::vector<Transaction>& history, std::string& message) const = 0;
};

class LargeAmountRule : public FraudRule {
public:
    std::string name() const override { return "Large amount"; }
    bool check(const Transaction& tx, const std::vector<Transaction>& history, std::string& message) const override;
};

class RapidTransactionsRule : public FraudRule {
public:
    std::string name() const override { return "Rapid transactions"; }
    bool check(const Transaction& tx, const std::vector<Transaction>& history, std::string& message) const override;
};

class OddHourRule : public FraudRule {
public:
    std::string name() const override { return "Odd hour"; }
    bool check(const Transaction& tx, const std::vector<Transaction>& history, std::string& message) const override;
};
