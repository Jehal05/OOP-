// Models.h - plain domain records: Transaction, Beneficiary, SavingsGoal, Alert.
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include <ostream>

enum class TxType { Deposit, Withdraw, TransferOut, TransferIn, Interest, LoanCredit, EMIPayment, GoalSave, Fee };
enum class Category { Food, Bills, Travel, Shopping, Rent, Health, Education, Other };

std::string toString(TxType t);
std::string toString(Category c);
bool isCredit(TxType t);                         // true => money in
const std::vector<TxType>& allTxTypes();
const std::vector<Category>& allCategories();

// ---------------------------------------------------------------- Transaction
class Transaction {
    std::string refId_, accNo_;
    TxType type_;
    Category cat_;
    double amount_, balanceAfter_;
    time_t ts_;
    std::string note_;
public:
    Transaction(std::string ref, std::string acc, TxType t, Category c, double amt,
                double balAfter, time_t ts, std::string note = "");
    const std::string& refId() const { return refId_; }
    const std::string& accNo() const { return accNo_; }
    TxType type() const { return type_; }
    Category category() const { return cat_; }
    double amount() const { return amount_; }
    double balanceAfter() const { return balanceAfter_; }
    time_t timestamp() const { return ts_; }
    const std::string& note() const { return note_; }
    bool credit() const { return isCredit(type_); }
    double signedAmount() const { return credit() ? amount_ : -amount_; }

    // Repository<T> contract
    std::string key() const { return refId_; }
    std::string toCsv() const;
    static std::unique_ptr<Transaction> fromCsv(const std::string& line);
    static const char* header() { return "refId,accNo,type,category,amount,balanceAfter,timestamp,note"; }

    bool operator<(const Transaction& o) const { return ts_ != o.ts_ ? ts_ < o.ts_ : refId_ < o.refId_; }
    friend std::ostream& operator<<(std::ostream& os, const Transaction& t);
};

// ---------------------------------------------------------------- Beneficiary
class Beneficiary {
    std::string owner_, nickname_, payeeAcc_, payeeName_;
    time_t addedOn_;
public:
    Beneficiary(std::string owner, std::string nick, std::string payeeAcc, std::string payeeName, time_t added);
    const std::string& owner() const { return owner_; }
    const std::string& nickname() const { return nickname_; }
    const std::string& payeeAcc() const { return payeeAcc_; }
    const std::string& payeeName() const { return payeeName_; }
    time_t addedOn() const { return addedOn_; }
    std::string key() const;                      // owner:nickname (lower-case)
    std::string toCsv() const;
    static std::unique_ptr<Beneficiary> fromCsv(const std::string& line);
    static const char* header() { return "owner,nickname,payeeAcc,payeeName,addedOn"; }
};

// ---------------------------------------------------------------- SavingsGoal
class SavingsGoal {
    std::string id_, accNo_, name_;
    double target_, saved_;
    time_t deadline_, createdOn_;
public:
    SavingsGoal(std::string id, std::string acc, std::string name, double target, double saved,
                time_t deadline, time_t created);
    const std::string& id() const { return id_; }
    const std::string& accNo() const { return accNo_; }
    const std::string& name() const { return name_; }
    double target() const { return target_; }
    double saved() const { return saved_; }
    time_t deadline() const { return deadline_; }
    double fraction() const { return target_ > 0 ? saved_ / target_ : 0; }
    std::string status(time_t now) const;         // Active / Achieved / Overdue
    double requiredMonthly(time_t now) const;     // amount to save each month to hit the deadline
    SavingsGoal& operator+=(double amount) { saved_ += amount; return *this; }

    std::string key() const { return id_; }
    std::string toCsv() const;
    static std::unique_ptr<SavingsGoal> fromCsv(const std::string& line);
    static const char* header() { return "id,accNo,name,target,saved,deadline,createdOn"; }
};

// ---------------------------------------------------------------- Alert (fraud)
class Alert {
    std::string id_, accNo_, refId_, rule_, message_;
    time_t ts_;
    bool reviewed_, seen_;
public:
    Alert(std::string id, std::string acc, std::string ref, std::string rule, std::string msg,
          time_t ts, bool reviewed = false, bool seen = false);
    const std::string& id() const { return id_; }
    const std::string& accNo() const { return accNo_; }
    const std::string& refId() const { return refId_; }
    const std::string& rule() const { return rule_; }
    const std::string& message() const { return message_; }
    time_t timestamp() const { return ts_; }
    bool reviewed() const { return reviewed_; }
    bool seen() const { return seen_; }
    void markReviewed() { reviewed_ = true; }
    void markSeen() { seen_ = true; }

    std::string key() const { return id_; }
    std::string toCsv() const;
    static std::unique_ptr<Alert> fromCsv(const std::string& line);
    static const char* header() { return "id,accNo,refId,rule,message,timestamp,reviewed,seen"; }
};
