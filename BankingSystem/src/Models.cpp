#include "Models.h"
#include "Utils.h"
#include <cmath>
#include <algorithm>

std::string toString(TxType t) {
    switch (t) {
        case TxType::Deposit: return "Deposit";
        case TxType::Withdraw: return "Withdraw";
        case TxType::TransferOut: return "Transfer Out";
        case TxType::TransferIn: return "Transfer In";
        case TxType::Interest: return "Interest";
        case TxType::LoanCredit: return "Loan Credit";
        case TxType::EMIPayment: return "EMI Payment";
        case TxType::GoalSave: return "Goal Saving";
        case TxType::Fee: return "Fee";
    }
    return "?";
}
std::string toString(Category c) {
    static const char* n[] = {"Food", "Bills", "Travel", "Shopping", "Rent", "Health", "Education", "Other"};
    return n[(int)c];
}
bool isCredit(TxType t) {
    return t == TxType::Deposit || t == TxType::TransferIn || t == TxType::Interest || t == TxType::LoanCredit;
}
const std::vector<TxType>& allTxTypes() {
    static const std::vector<TxType> v{TxType::Deposit, TxType::Withdraw, TxType::TransferOut, TxType::TransferIn,
                                       TxType::Interest, TxType::LoanCredit, TxType::EMIPayment, TxType::GoalSave, TxType::Fee};
    return v;
}
const std::vector<Category>& allCategories() {
    static const std::vector<Category> v{Category::Food, Category::Bills, Category::Travel, Category::Shopping,
                                         Category::Rent, Category::Health, Category::Education, Category::Other};
    return v;
}

// ------------------------------------------------------------------ Transaction
Transaction::Transaction(std::string ref, std::string acc, TxType t, Category c, double amt,
                         double balAfter, time_t ts, std::string note)
    : refId_(std::move(ref)), accNo_(std::move(acc)), type_(t), cat_(c), amount_(amt),
      balanceAfter_(balAfter), ts_(ts), note_(util::cleanText(note)) {}

std::string Transaction::toCsv() const {
    char amt[32], bal[32];
    std::snprintf(amt, sizeof amt, "%.2f", amount_);
    std::snprintf(bal, sizeof bal, "%.2f", balanceAfter_);
    return util::join({refId_, accNo_, std::to_string((int)type_), std::to_string((int)cat_), amt, bal,
                       util::fmtDateTime(ts_), util::csvEscape(note_)}, ',');
}
std::unique_ptr<Transaction> Transaction::fromCsv(const std::string& line) {
    auto p = util::split(line, ',');
    time_t ts;
    if (p.size() < 7 || !util::parseDateTime(p[6], ts)) return nullptr;
    return std::make_unique<Transaction>(p[0], p[1], (TxType)std::stoi(p[2]), (Category)std::stoi(p[3]),
                                         std::stod(p[4]), std::stod(p[5]), ts, p.size() > 7 ? util::csvUnescape(p[7]) : "");
}
std::ostream& operator<<(std::ostream& os, const Transaction& t) {
    return os << t.refId_ << " | " << util::fmtDateTime(t.ts_) << " | " << toString(t.type_) << " | "
              << (t.credit() ? "+" : "-") << util::money(t.amount_) << " | bal " << util::money(t.balanceAfter_);
}

// ------------------------------------------------------------------ Beneficiary
Beneficiary::Beneficiary(std::string owner, std::string nick, std::string payeeAcc, std::string payeeName, time_t added)
    : owner_(std::move(owner)), nickname_(util::sanitize(nick)), payeeAcc_(std::move(payeeAcc)),
      payeeName_(util::sanitize(payeeName)), addedOn_(added) {}
std::string Beneficiary::key() const { return owner_ + ":" + util::lower(nickname_); }
std::string Beneficiary::toCsv() const {
    return util::join({owner_, nickname_, payeeAcc_, payeeName_, util::fmtDateTime(addedOn_)}, ',');
}
std::unique_ptr<Beneficiary> Beneficiary::fromCsv(const std::string& line) {
    auto p = util::split(line, ',');
    time_t t;
    if (p.size() < 5 || !util::parseDateTime(p[4], t)) return nullptr;
    return std::make_unique<Beneficiary>(p[0], p[1], p[2], p[3], t);
}

// ------------------------------------------------------------------ SavingsGoal
SavingsGoal::SavingsGoal(std::string id, std::string acc, std::string name, double target, double saved,
                         time_t deadline, time_t created)
    : id_(std::move(id)), accNo_(std::move(acc)), name_(util::sanitize(name)), target_(target), saved_(saved),
      deadline_(deadline), createdOn_(created) {}
std::string SavingsGoal::status(time_t now) const {
    if (saved_ + 0.005 >= target_) return "Achieved";
    return now > deadline_ ? "Overdue" : "Active";
}
double SavingsGoal::requiredMonthly(time_t now) const {
    double remaining = std::max(0.0, target_ - saved_);
    int months = std::max(1, util::monthsBetween(now, deadline_));
    return remaining / months;
}
std::string SavingsGoal::toCsv() const {
    char t[32], s[32];
    std::snprintf(t, sizeof t, "%.2f", target_);
    std::snprintf(s, sizeof s, "%.2f", saved_);
    return util::join({id_, accNo_, name_, t, s, util::fmtDate(deadline_), util::fmtDateTime(createdOn_)}, ',');
}
std::unique_ptr<SavingsGoal> SavingsGoal::fromCsv(const std::string& line) {
    auto p = util::split(line, ',');
    time_t dl, cr;
    if (p.size() < 7 || !util::parseDate(p[5], dl) || !util::parseDateTime(p[6], cr)) return nullptr;
    return std::make_unique<SavingsGoal>(p[0], p[1], p[2], std::stod(p[3]), std::stod(p[4]), dl, cr);
}

// ------------------------------------------------------------------ Alert
Alert::Alert(std::string id, std::string acc, std::string ref, std::string rule, std::string msg,
             time_t ts, bool reviewed, bool seen)
    : id_(std::move(id)), accNo_(std::move(acc)), refId_(std::move(ref)), rule_(std::move(rule)),
      message_(util::cleanText(msg)), ts_(ts), reviewed_(reviewed), seen_(seen) {}
std::string Alert::toCsv() const {
    return util::join({id_, accNo_, refId_, rule_, util::csvEscape(message_), util::fmtDateTime(ts_),
                       reviewed_ ? "1" : "0", seen_ ? "1" : "0"}, ',');
}
std::unique_ptr<Alert> Alert::fromCsv(const std::string& line) {
    auto p = util::split(line, ',');
    time_t ts;
    if (p.size() < 8 || !util::parseDateTime(p[5], ts)) return nullptr;
    return std::make_unique<Alert>(p[0], p[1], p[2], p[3], util::csvUnescape(p[4]), ts, p[6] == "1", p[7] == "1");
}
