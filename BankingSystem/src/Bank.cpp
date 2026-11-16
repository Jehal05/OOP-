#include "Bank.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

namespace {
double round2(double v) { return std::round(v * 100.0) / 100.0; }
const char* ADMIN_ID = "admin";
}

Bank::Bank(std::string dataDir) : dir_(std::move(dataDir)) {
    users_.setPath(dir_ + "/users.csv");
    accounts_.setPath(dir_ + "/accounts.csv");
    txns_.setPath(dir_ + "/transactions.csv");
    payees_.setPath(dir_ + "/beneficiaries.csv");
    goals_.setPath(dir_ + "/goals.csv");
    loans_.setPath(dir_ + "/loans.csv");
    alerts_.setPath(dir_ + "/alerts.csv");
    audit_.setPath(dir_ + "/audit.log");
}

void Bank::load() {
    users_.load(); accounts_.load(); txns_.load(); payees_.load(); goals_.load(); loans_.load(); alerts_.load();
    for (const auto& t : txns_.all()) {                       // continue the reference counter after the last record
        const std::string& r = t->refId();
        if (r.size() >= 6 && util::isDigits(r.substr(r.size() - 6))) refCounter_ = std::max(refCounter_, std::stoi(r.substr(r.size() - 6)));
    }
    if (users_.empty() && accounts_.empty()) seedDemoData();
}

void Bank::saveAll() const {
    users_.saveAll(); accounts_.saveAll(); txns_.saveAll(); payees_.saveAll(); goals_.saveAll(); loans_.saveAll(); alerts_.saveAll();
}

// ================================================================ internals
std::string Bank::nextRef(time_t when) {
    char buf[40];
    std::snprintf(buf, sizeof buf, "TXN-%s-%06d", util::compactDate(when).c_str(), ++refCounter_);
    return buf;
}

std::string Bank::nextId(const char* prefix, int width, const std::vector<std::string>& existing) const {
    int mx = 0;
    for (const auto& e : existing) if (e.size() > 1 && util::isDigits(e.substr(1))) mx = std::max(mx, std::stoi(e.substr(1)));
    char buf[32];
    std::snprintf(buf, sizeof buf, "%s%0*d", prefix, width, mx + 1);
    return buf;
}

const Transaction& Bank::record(Account& a, TxType type, Category cat, double amount, double balanceAfter,
                                const std::string& note, double balanceBefore, bool newPayee, bool screen, time_t when) {
    if (when == 0) when = util::now();
    Transaction* tx = txns_.addAndAppend(std::make_unique<Transaction>(nextRef(when), a.accNo(), type, cat, amount, balanceAfter, when, note));
    accounts_.patch(a.accNo());                                // in-place balance update (seekp)
    if (screen) screenFraud(*tx, balanceBefore, newPayee);
    return *tx;
}

const Transaction& Bank::debit(Account& a, TxType type, Category cat, double amount, const std::string& note,
                               bool screen, bool newPayee) {
    time_t now = util::now();
    double fee = (type == TxType::Withdraw || type == TxType::TransferOut) ? a.withdrawalFee(amount, now) : 0.0;
    double before = a.balance();
    try {
        a.withdraw(amount, fee);
    } catch (const BankException&) {
        ++failedOps_[a.accNo()];
        throw;
    }
    const Transaction& tx = record(a, type, cat, amount, round2(before - amount), note, before, newPayee, screen);
    if (fee > 0) record(a, TxType::Fee, Category::Other, fee, a.balance(), "Early FD withdrawal penalty (1%)", before, false, false);
    if (!screen) failedOps_[a.accNo()] = 0;
    return tx;
}

const Transaction& Bank::credit(Account& a, TxType type, double amount, const std::string& note, bool screen) {
    double before = a.balance();
    a.deposit(amount);
    return record(a, type, Category::Other, amount, a.balance(), note, before, false, screen);
}

void Bank::screenFraud(const Transaction& tx, double balanceBefore, bool newPayee) {
    FraudContext ctx;
    ctx.balanceBefore = balanceBefore;
    ctx.newPayee = newPayee;
    ctx.failedOps = failedOps_[tx.accNo()];
    for (const auto& t : txns_.all())
        if (t->accNo() == tx.accNo() && t->refId() != tx.refId() && t->timestamp() <= tx.timestamp() &&
            tx.timestamp() - t->timestamp() <= 600)
            ctx.recent.push_back(*t);
    for (const auto& [rule, msg] : fraud_.evaluate(tx, ctx)) {
        std::vector<std::string> ids;
        for (const auto& a : alerts_.all()) ids.push_back(a->id());
        alerts_.addAndAppend(std::make_unique<Alert>(nextId("A", 3, ids), tx.accNo(), tx.refId(), rule, msg, tx.timestamp()));
        audit_.log("SYSTEM", "FRAUD_ALERT", tx.accNo() + " " + tx.refId() + " " + rule);
    }
    failedOps_[tx.accNo()] = 0;
}

// ================================================================ authentication
User& Bank::login(const std::string& rawId, const std::string& pin, const std::string& role) {
    std::string id = util::upper(util::trim(rawId));
    User* u = users_.find(role == "ADMIN" ? util::lower(id) : id);
    if (!u || u->role() != role) {
        audit_.log(rawId.empty() ? "?" : rawId, "LOGIN_FAILED", "unknown id (" + role + ")");
        throw AuthenticationException();
    }
    if (u->locked()) {
        audit_.log(u->id(), "LOGIN_BLOCKED", "account locked");
        throw AccountLockedException();
    }
    if (!u->verifyPin(pin)) {
        u->recordFailure();
        users_.patch(u->id());
        audit_.log(u->id(), "LOGIN_FAILED", "wrong PIN, failures=" + std::to_string(u->failedAttempts()));
        if (u->locked()) {
            audit_.log("SYSTEM", "ACCOUNT_LOCKED", u->id());
            throw AccountLockedException();
        }
        throw AuthenticationException(u->attemptsLeft());
    }
    if (u->role() == "CUSTOMER") {
        Account* a = accounts_.find(u->linkedAccount());
        if (!a || !a->active()) throw BankException("This account has been closed. Please visit a branch.");
    }
    u->resetFailures();
    users_.patch(u->id());
    audit_.log(u->id(), "LOGIN_SUCCESS", u->role());
    return *u;
}

void Bank::changePin(User& user, const std::string& oldPin, const std::string& newPin) {
    if (!user.verifyPin(oldPin)) throw AuthenticationException();
    if (newPin.size() != 4 || !util::isDigits(newPin)) throw BankException("The new PIN must be exactly 4 digits.");
    if (newPin == oldPin) throw BankException("The new PIN must be different from the old PIN.");
    user.setPin(newPin);
    users_.patch(user.id());
    audit_.log(user.id(), "PIN_CHANGED");
}

void Bank::unlockUser(const std::string& adminId, const std::string& userId) {
    User* u = users_.find(userId);
    if (!u) throw AccountNotFoundException(userId);
    u->unlock();
    users_.patch(userId);
    audit_.log(adminId, "UNLOCK_ACCOUNT", userId);
}

std::vector<User*> Bank::lockedUsers() const { return users_.where([](const User& u) { return u.locked(); }); }

// ================================================================ accounts
Account& Bank::account(const std::string& accNo) const {
    Account* a = accounts_.find(util::upper(util::trim(accNo)));
    if (!a) throw AccountNotFoundException(accNo);
    return *a;
}
std::vector<Account*> Bank::allAccounts() const { return accounts_.where([](const Account&) { return true; }); }

std::vector<Account*> Bank::searchAccounts(const std::string& q) const {
    std::string k = util::lower(util::trim(q));
    return accounts_.where([&](const Account& a) {
        return util::lower(a.accNo()).find(k) != std::string::npos || util::lower(a.holder()).find(k) != std::string::npos;
    });
}

std::string Bank::openAccount(const std::string& adminId, const std::string& code, const std::string& holder,
                              double initial, const std::string& pin, double p1, double p2) {
    std::string name = util::sanitize(holder);
    if (name.size() < 2) throw BankException("Holder name must have at least 2 characters.");
    if (pin.size() != 4 || !util::isDigits(pin)) throw BankException("The PIN must be exactly 4 digits.");
    struct Rule { const char* prefix; int base; double minOpen; };
    std::map<std::string, Rule> rules{{"SAV", {"SB", 1001, 1000}}, {"STU", {"ST", 1001, 0}},
                                      {"CUR", {"CA", 2001, 5000}}, {"FD", {"FD", 3001, 10000}}};
    auto it = rules.find(code);
    if (it == rules.end()) throw BankException("Unknown account type.");
    if (initial < it->second.minOpen)
        throw InvalidAmountException("minimum opening deposit is " + util::money(it->second.minOpen));
    int mx = it->second.base - 1;
    for (const auto& a : accounts_.all())
        if (a->accNo().rfind(it->second.prefix, 0) == 0) mx = std::max(mx, std::stoi(a->accNo().substr(2)));
    std::string accNo = std::string(it->second.prefix) + std::to_string(mx + 1);
    time_t now = util::now();
    std::unique_ptr<Account> acc;
    if (code == "SAV") acc = std::make_unique<SavingsAccount>(accNo, name, 0, now, p1, p2);
    else if (code == "STU") acc = std::make_unique<StudentSavingsAccount>(accNo, name, 0, now, p1);
    else if (code == "CUR") acc = std::make_unique<CurrentAccount>(accNo, name, 0, now, p1);
    else acc = std::make_unique<FixedDepositAccount>(accNo, name, 0, now, p1, (int)p2);
    Account* a = accounts_.add(std::move(acc));
    accounts_.saveAll();
    users_.addAndAppend(std::make_unique<Customer>(accNo, name, pin, accNo));
    if (initial > 0) credit(*a, TxType::Deposit, initial, "Opening deposit", false);
    audit_.log(adminId, "ACCOUNT_OPENED", accNo + " " + a->typeName() + " " + name);
    return accNo;
}

void Bank::updateHolder(const std::string& adminId, const std::string& accNo, const std::string& newName) {
    Account& a = account(accNo);
    std::string n = util::sanitize(newName);
    if (n.size() < 2) throw BankException("Holder name must have at least 2 characters.");
    std::string old = a.holder();
    a.setHolder(n);
    accounts_.patch(a.accNo());
    audit_.log(adminId, "ACCOUNT_UPDATED", a.accNo() + " name '" + old + "' -> '" + n + "'");
}

double Bank::closeAccount(const std::string& adminId, const std::string& accNo) {
    Account& a = account(accNo);
    if (!a.active()) throw BankException("Account is already closed.");
    if (a.balance() < 0) throw BankException("Cannot close: overdraft of " + util::money(-a.balance()) + " must be cleared first.");
    for (const Loan* l : loansOf(a.accNo()))
        if (l->status() == LoanStatus::Active || l->status() == LoanStatus::Pending)
            throw BankException("Cannot close: loan " + l->id() + " is still " + toString(l->status()) + ".");
    double payout = a.closeAndPayout();
    if (payout > 0) record(a, TxType::Withdraw, Category::Other, payout, 0.0, "Account closure payout", payout, false, false);
    else accounts_.patch(a.accNo());
    audit_.log(adminId, "ACCOUNT_CLOSED", a.accNo() + " payout " + util::number(payout));
    return payout;
}

// ================================================================ money movement
const Transaction& Bank::deposit(const std::string& accNo, double amount, const std::string& note) {
    Account& a = account(accNo);
    const Transaction& tx = credit(a, TxType::Deposit, amount, note, true);
    return tx;
}

const Transaction& Bank::withdraw(const std::string& accNo, double amount, Category cat, const std::string& note) {
    Account& a = account(accNo);
    return debit(a, TxType::Withdraw, cat, amount, note.empty() ? "Cash withdrawal" : note, true);
}

const Transaction& Bank::transfer(const std::string& from, const std::string& to, double amount, Category cat,
                                  const std::string& note) {
    Account& src = account(from);
    Account& dst = account(to);
    if (src.accNo() == dst.accNo()) throw InvalidAmountException("cannot transfer to the same account");
    if (!dst.active()) throw BankException("The destination account is closed.");
    bool newPayee = false;
    for (const Beneficiary* b : payeesOf(src.accNo()))
        if (b->payeeAcc() == dst.accNo() && util::now() - b->addedOn() < 86400) newPayee = true;
    std::string n = note.empty() ? "To " + dst.holder() + " (" + util::maskAccount(dst.accNo()) + ")" : note;
    const Transaction& out = debit(src, TxType::TransferOut, cat, amount, n, true, newPayee);
    credit(dst, TxType::TransferIn, amount, "From " + src.holder() + " (" + util::maskAccount(src.accNo()) + ")", false);
    return out;
}

std::vector<Transaction> Bank::history(const std::string& accNo) const {
    std::vector<Transaction> out;
    for (const auto& t : txns_.all()) if (t->accNo() == accNo) out.push_back(*t);
    std::sort(out.begin(), out.end());
    return out;
}

const Transaction* Bank::findTransaction(const std::string& refId) const { return txns_.find(util::upper(util::trim(refId))); }

double Bank::fdWithdrawalFee(const std::string& accNo, double amount) const {
    return account(accNo).withdrawalFee(amount, util::now());
}

// ================================================================ beneficiaries
const Beneficiary& Bank::addPayee(const std::string& owner, const std::string& nick, const std::string& payeeAccRaw) {
    std::string nickname = util::sanitize(nick), payeeAcc = util::upper(util::trim(payeeAccRaw));
    if (nickname.empty() || nickname.size() > 15) throw BankException("Nickname must be 1-15 characters.");
    if (payeeAcc == owner) throw BankException("You cannot add your own account as a beneficiary.");
    Account& pa = account(payeeAcc);
    if (!pa.active()) throw BankException("That account is closed.");
    if (findPayee(owner, nickname)) throw BankException("A beneficiary named '" + nickname + "' already exists.");
    for (const Beneficiary* b : payeesOf(owner))
        if (b->payeeAcc() == payeeAcc) throw BankException("This account is already saved as '" + b->nickname() + "'.");
    Beneficiary* b = payees_.add(std::make_unique<Beneficiary>(owner, nickname, payeeAcc, pa.holder(), util::now()));
    payees_.saveAll();
    audit_.log(owner, "PAYEE_ADDED", nickname + " -> " + payeeAcc);
    return *b;
}

void Bank::removePayee(const std::string& owner, const std::string& nickname) {
    const Beneficiary* b = findPayee(owner, nickname);
    if (!b) throw BankException("No beneficiary named '" + nickname + "'.");
    payees_.remove(owner + ":" + util::lower(util::sanitize(nickname)));
    payees_.saveAll();
    audit_.log(owner, "PAYEE_REMOVED", nickname);
}

std::vector<Beneficiary*> Bank::payeesOf(const std::string& owner) const {
    return payees_.where([&](const Beneficiary& b) { return b.owner() == owner; });
}
const Beneficiary* Bank::findPayee(const std::string& owner, const std::string& nickname) const {
    return payees_.find(owner + ":" + util::lower(util::sanitize(nickname)));
}

// ================================================================ goals
const SavingsGoal& Bank::createGoal(const std::string& accNo, const std::string& name, double target, time_t deadline) {
    std::string n = util::sanitize(name);
    if (n.empty() || n.size() > 24) throw BankException("Goal name must be 1-24 characters.");
    if (!(target > 0)) throw InvalidAmountException("target must be greater than zero");
    if (deadline <= util::now()) throw BankException("The deadline must be in the future.");
    std::vector<std::string> ids;
    for (const auto& g : goals_.all()) ids.push_back(g->id());
    SavingsGoal* g = goals_.add(std::make_unique<SavingsGoal>(nextId("G", 3, ids), accNo, n, target, 0.0, deadline, util::now()));
    goals_.saveAll();
    audit_.log(accNo, "GOAL_CREATED", g->id() + " " + n);
    return *g;
}

const Transaction& Bank::contributeToGoal(const std::string& goalId, double amount) {
    SavingsGoal* g = goals_.find(goalId);
    if (!g) throw BankException("Goal not found.");
    double remaining = g->target() - g->saved();
    if (remaining <= 0.005) throw BankException("This goal is already achieved.");
    if (amount > remaining + 0.005) throw InvalidAmountException("this goal needs only " + util::money(remaining) + " more");
    Account& a = account(g->accNo());
    const Transaction& tx = debit(a, TxType::GoalSave, Category::Other, amount, "Goal: " + g->name(), false);
    *g += amount;                                                       // SavingsGoal::operator+=
    goals_.saveAll();
    return tx;
}

std::vector<SavingsGoal*> Bank::goalsOf(const std::string& accNo) const {
    return goals_.where([&](const SavingsGoal& g) { return g.accNo() == accNo; });
}

// ================================================================ loans
const Loan& Bank::applyLoan(const std::string& accNo, const std::string& code, double principal, int months) {
    Account& a = account(accNo);
    if (!a.active()) throw LoanException("Account is closed.");
    auto probe = Loan::create(code, "", accNo, principal, months, LoanStatus::Pending, 0, util::now());
    if (!probe) throw LoanException("Unknown loan type.");
    if (principal < 10000) throw LoanException("Minimum loan amount is " + util::money(10000) + ".");
    if (principal > probe->maxPrincipal()) throw LoanException(probe->typeName() + " limit is " + util::money(probe->maxPrincipal()) + ".");
    if (months < 6 || months > probe->maxMonths())
        throw LoanException("Tenure for a " + probe->typeName() + " must be between 6 and " + std::to_string(probe->maxMonths()) + " months.");
    int open = 0;
    for (const Loan* l : loansOf(accNo)) if (l->status() == LoanStatus::Active || l->status() == LoanStatus::Pending) ++open;
    if (open >= 2) throw LoanException("You already have 2 open loans. Close one before applying again.");
    std::vector<std::string> ids;
    for (const auto& l : loans_.all()) ids.push_back(l->id());
    Loan* l = loans_.add(Loan::create(code, nextId("L", 3, ids), accNo, principal, months, LoanStatus::Pending, 0, util::now()));
    loans_.saveAll();
    audit_.log(accNo, "LOAN_APPLIED", l->id() + " " + l->typeName() + " " + util::number(principal));
    return *l;
}

void Bank::approveLoan(const std::string& adminId, const std::string& loanId) {
    Loan* l = loans_.find(loanId);
    if (!l) throw LoanException("Loan not found.");
    if (l->status() != LoanStatus::Pending) throw LoanException("Only pending loans can be approved.");
    Account& a = account(l->accNo());
    l->setStatus(LoanStatus::Active);
    credit(a, TxType::LoanCredit, l->principal(), "Loan " + l->id() + " disbursed", false);
    loans_.saveAll();
    audit_.log(adminId, "LOAN_APPROVED", l->id() + " " + l->accNo());
}

void Bank::rejectLoan(const std::string& adminId, const std::string& loanId, const std::string& reason) {
    Loan* l = loans_.find(loanId);
    if (!l) throw LoanException("Loan not found.");
    if (l->status() != LoanStatus::Pending) throw LoanException("Only pending loans can be rejected.");
    l->setStatus(LoanStatus::Rejected);
    l->setNote(reason.empty() ? "Rejected by admin" : reason);
    loans_.saveAll();
    audit_.log(adminId, "LOAN_REJECTED", l->id() + " " + l->note());
}

const Transaction& Bank::payEMI(const std::string& loanId) {
    Loan* l = loans_.find(loanId);
    if (!l || l->status() != LoanStatus::Active) throw LoanException("Loan is not active.");
    double amt = l->nextInstallment();
    Account& a = account(l->accNo());
    const Transaction& tx = debit(a, TxType::EMIPayment, Category::Other, amt,
                                  "EMI " + std::to_string(l->paid() + 1) + "/" + std::to_string(l->months()) + " loan " + l->id(), false);
    l->incrementPaid();
    if (l->paid() >= l->months()) l->setStatus(LoanStatus::Closed);
    loans_.saveAll();
    return tx;
}

const Transaction& Bank::prepayLoan(const std::string& loanId) {
    Loan* l = loans_.find(loanId);
    if (!l || l->status() != LoanStatus::Active) throw LoanException("Loan is not active.");
    double amt = l->outstanding();
    Account& a = account(l->accNo());
    const Transaction& tx = debit(a, TxType::EMIPayment, Category::Other, amt, "Prepayment (full closure) loan " + l->id(), false);
    l->setStatus(LoanStatus::Closed);
    loans_.saveAll();
    audit_.log(l->accNo(), "LOAN_PREPAID", l->id() + " " + util::number(amt));
    return tx;
}

std::vector<Loan*> Bank::loansOf(const std::string& accNo) const {
    return loans_.where([&](const Loan& l) { return l.accNo() == accNo; });
}
std::vector<Loan*> Bank::pendingLoans() const {
    return loans_.where([](const Loan& l) { return l.status() == LoanStatus::Pending; });
}

// ================================================================ alerts
std::vector<Alert*> Bank::unseenAlerts(const std::string& accNo) const {
    return alerts_.where([&](const Alert& a) { return a.accNo() == accNo && !a.seen(); });
}
std::vector<Alert*> Bank::alertsOf(const std::string& accNo) const {
    return alerts_.where([&](const Alert& a) { return a.accNo() == accNo; });
}
void Bank::markAlertsSeen(const std::string& accNo) {
    for (Alert* a : unseenAlerts(accNo)) a->markSeen();
    alerts_.saveAll();
}
std::vector<Alert*> Bank::alertsAwaitingReview() const {
    return alerts_.where([](const Alert& a) { return !a.reviewed(); });
}
void Bank::markAlertReviewed(const std::string& adminId, const std::string& alertId) {
    Alert* a = alerts_.find(alertId);
    if (!a) throw BankException("Alert not found.");
    a->markReviewed();
    alerts_.saveAll();
    audit_.log(adminId, "ALERT_REVIEWED", alertId + " " + a->rule());
}

// ================================================================ interest / simulator / reports
std::vector<InterestResult> Bank::applyMonthlyInterest(const std::string& adminId) {
    std::vector<InterestResult> out;
    std::string month = util::monthKey(util::now());
    for (Account* a : allAccounts()) {
        std::string code = a->typeCode();
        if (!a->active() || (code != "SAV" && code != "STU")) continue;
        bool already = false;
        for (const auto& t : txns_.all())
            if (t->accNo() == a->accNo() && t->type() == TxType::Interest && util::monthKey(t->timestamp()) == month) already = true;
        double interest = round2(a->balance() * a->annualRate() / 1200.0);
        if (already) { out.push_back({a->accNo(), a->holder(), 0, "Already credited this month"}); continue; }
        if (interest <= 0) { out.push_back({a->accNo(), a->holder(), 0, "No interest (zero balance)"}); continue; }
        credit(*a, TxType::Interest, interest, "Monthly interest " + month, false);
        out.push_back({a->accNo(), a->holder(), interest, "Credited"});
    }
    audit_.log(adminId, "INTEREST_APPLIED", month);
    return out;
}

std::vector<SimRow> Bank::simulate(const Account& a, double principal, int years) {
    std::vector<SimRow> rows;
    double r = a.annualRate() / 100.0;
    for (int y = 1; y <= years; ++y) {
        double simple = principal * (1.0 + r * y);
        double compound = principal * std::pow(1.0 + r, y);
        rows.push_back({y, round2(simple), round2(compound)});
    }
    return rows;
}

double Bank::totalDeposits() const {
    return std::accumulate(txns_.all().begin(), txns_.all().end(), 0.0,
        [](double s, const std::unique_ptr<Transaction>& t) { return s + (t->type() == TxType::Deposit ? t->amount() : 0.0); });
}
double Bank::totalBalance() const {
    return std::accumulate(accounts_.all().begin(), accounts_.all().end(), 0.0,
        [](double s, const std::unique_ptr<Account>& a) { return s + (a->active() ? a->balance() : 0.0); });
}
std::vector<Account*> Bank::richest(size_t n) const {
    auto v = accounts_.where([](const Account& a) { return a.active(); });
    std::sort(v.begin(), v.end(), [](const Account* a, const Account* b) { return a->balance() > b->balance(); });
    if (v.size() > n) v.resize(n);
    return v;
}
std::vector<std::pair<Account*, time_t>> Bank::dormant(int days) const {
    std::map<std::string, time_t> last;
    for (const auto& t : txns_.all()) last[t->accNo()] = std::max(last[t->accNo()], t->timestamp());
    std::vector<std::pair<Account*, time_t>> out;
    time_t cutoff = util::now() - (time_t)days * 86400;
    for (Account* a : allAccounts()) {
        if (!a->active()) continue;
        time_t l = last.count(a->accNo()) ? last[a->accNo()] : a->openedOn();
        if (l < cutoff) out.emplace_back(a, l);
    }
    return out;
}
std::map<std::string, std::pair<double, double>> Bank::periodTotals(bool monthly) const {
    std::map<std::string, std::pair<double, double>> m;
    for (const auto& t : txns_.all()) {
        std::string k = monthly ? util::monthKey(t->timestamp()) : util::fmtDate(t->timestamp());
        if (t->credit()) m[k].first += t->amount(); else m[k].second += t->amount();
    }
    return m;
}

// ================================================================ demo data
void Bank::seedDemoData() {
    time_t now = util::now();
    auto at = [&](int daysAgo, int hour, int minute) {
        time_t d = util::addDays(now, -daysAgo);
        std::tm tmv{};
#ifdef _WIN32
        localtime_s(&tmv, &d);
#else
        localtime_r(&d, &tmv);
#endif
        tmv.tm_hour = hour; tmv.tm_min = minute; tmv.tm_sec = 0; tmv.tm_isdst = -1;
        return std::mktime(&tmv);
    };

    // ---- accounts and users (balances are built up from the seed transactions below) ----
    accounts_.add(std::make_unique<SavingsAccount>("SB1001", "Asha Verma", 0, at(120, 10, 0), 4.0, 1000.0));
    accounts_.add(std::make_unique<StudentSavingsAccount>("ST1001", "Arjun Rao", 0, at(45, 11, 0), 4.5));
    accounts_.add(std::make_unique<CurrentAccount>("CA2001", "Ravi Kumar", 0, at(60, 10, 30), 25000.0));
    accounts_.add(std::make_unique<FixedDepositAccount>("FD3001", "Meera Nair", 0, at(100, 12, 0), 7.0, 12));
    users_.add(std::make_unique<Customer>("SB1001", "Asha Verma", "1234", "SB1001"));
    users_.add(std::make_unique<Customer>("ST1001", "Arjun Rao", "4567", "ST1001"));
    users_.add(std::make_unique<Customer>("CA2001", "Ravi Kumar", "2345", "CA2001"));
    users_.add(std::make_unique<Customer>("FD3001", "Meera Nair", "3456", "FD3001"));
    users_.add(std::make_unique<Admin>(ADMIN_ID, "Bank Administrator", "admin123"));

    // ---- loan L001 (active, 3 EMIs paid) so its EMI is known before seeding the ledger ----
    auto l1 = Loan::create("PER", "L001", "SB1001", 100000, 12, LoanStatus::Active, 3, at(95, 10, 0));
    double emi = l1->emi();

    struct S { const char* acc; int d, h, m; TxType t; Category c; double amt; const char* note; };
    const TxType DEP = TxType::Deposit, WD = TxType::Withdraw;
    const Category O = Category::Other;
    std::vector<S> seed = {
        {"SB1001",120,10,0,DEP,O,100000,"Opening deposit"},
        {"SB1001",95,10,5,TxType::LoanCredit,O,100000,"Loan L001 disbursed"},
        {"SB1001",90,10,0,WD,Category::Rent,15000,"House rent"},
        {"SB1001",88,18,30,WD,Category::Food,1800,"Groceries"},
        {"SB1001",85,9,15,WD,Category::Bills,2300,"Electricity bill"},
        {"SB1001",80,14,0,DEP,O,55000,"Salary"},
        {"SB1001",74,19,45,WD,Category::Shopping,4200,"Clothes"},
        {"SB1001",70,13,10,WD,Category::Travel,6400,"Train tickets"},
        {"SB1001",65,10,0,TxType::EMIPayment,O,emi,"EMI 1/12 loan L001"},
        {"SB1001",58,16,20,WD,Category::Health,1200,"Pharmacy"},
        {"SB1001",57,18,0,WD,Category::Food,2100,"Groceries"},
        {"SB1001",52,11,0,WD,Category::Rent,15000,"House rent"},
        {"SB1001",50,12,0,DEP,O,55000,"Salary"},
        {"SB1001",45,9,40,WD,Category::Bills,1800,"Internet and mobile"},
        {"SB1001",40,17,25,WD,Category::Education,8000,"Online course"},
        {"SB1001",36,20,10,WD,Category::Food,1450,"Groceries"},
        {"SB1001",35,10,0,TxType::EMIPayment,O,emi,"EMI 2/12 loan L001"},
        {"SB1001",30,15,35,WD,Category::Shopping,6800,"Electronics"},
        {"SB1001",25,11,0,WD,Category::Rent,15000,"House rent"},
        {"SB1001",22,10,0,TxType::Interest,O,290,"Monthly interest"},
        {"SB1001",20,13,15,WD,Category::Food,1250,"Restaurant"},
        {"SB1001",16,18,50,WD,Category::Travel,3500,"Cab and fuel"},
        {"SB1001",14,12,0,DEP,O,55000,"Salary"},
        {"SB1001",10,11,30,WD,Category::Health,2500,"Doctor visit"},
        {"SB1001",9,15,0,TxType::GoalSave,O,22000,"Goal: Laptop"},
        {"SB1001",7,19,0,WD,Category::Bills,2650,"Credit card bill"},
        {"SB1001",6,10,0,WD,Category::Food,1900,"Groceries"},
        {"SB1001",5,10,0,TxType::EMIPayment,O,emi,"EMI 3/12 loan L001"},
        {"SB1001",4,16,0,TxType::GoalSave,O,45000,"Goal: Emergency Fund"},
        {"SB1001",3,11,30,TxType::TransferOut,Category::Rent,15000,"To Ravi Kumar (XXXX-2001)"},
        {"SB1001",2,17,15,WD,Category::Shopping,3200,"Festival shopping"},
        {"SB1001",1,9,45,WD,Category::Food,780,"Cafe"},

        {"CA2001",60,10,30,DEP,O,60000,"Opening deposit"},
        {"CA2001",50,12,0,WD,Category::Shopping,12000,"Office supplies"},
        {"CA2001",45,15,0,DEP,O,35000,"Client payment"},
        {"CA2001",40,11,0,WD,Category::Rent,20000,"Shop rent"},
        {"CA2001",35,10,30,WD,Category::Bills,8500,"Electricity and water"},
        {"CA2001",28,16,0,DEP,O,42000,"Client payment"},
        {"CA2001",22,2,40,WD,Category::Travel,9800,"Airport cab and hotel advance"},
        {"CA2001",15,13,30,WD,Category::Food,3200,"Team lunch"},
        {"CA2001",9,15,0,DEP,O,28000,"Client payment"},
        {"CA2001",3,11,30,TxType::TransferIn,O,15000,"From Asha Verma (XXXX-1001)"},
        {"CA2001",1,14,0,WD,Category::Bills,5600,"Vendor payment"},

        {"FD3001",100,12,0,DEP,O,200000,"Opening deposit - fixed deposit"},

        {"ST1001",45,11,0,DEP,O,15000,"Opening deposit"},
        {"ST1001",40,12,0,DEP,O,5000,"Pocket money"},
        {"ST1001",35,14,0,WD,Category::Education,2500,"Books"},
        {"ST1001",30,13,0,WD,Category::Food,1800,"Mess and snacks"},
        {"ST1001",20,17,0,WD,Category::Shopping,1500,"Headphones"},
        {"ST1001",12,15,30,WD,Category::Travel,900,"Bus pass"},
        {"ST1001",7,10,0,DEP,O,3000,"Scholarship"},
        {"ST1001",2,13,0,WD,Category::Food,650,"Lunch"},
    };
    std::stable_sort(seed.begin(), seed.end(), [&](const S& a, const S& b) { return at(a.d, a.h, a.m) < at(b.d, b.h, b.m); });
    std::string oddHourRef;
    for (const S& e : seed) {
        Account* a = accounts_.find(e.acc);
        time_t when = at(e.d, e.h, e.m);
        double after = round2(a->balance() + (isCredit(e.t) ? e.amt : -e.amt));
        a->restoreBalance(after);
        const std::string ref = nextRef(when);
        txns_.add(std::make_unique<Transaction>(ref, e.acc, e.t, e.c, round2(e.amt), after, when, e.note));
        if (std::string(e.note) == "Airport cab and hotel advance") oddHourRef = ref;
    }

    // ---- loans: L001 active (3 EMIs paid, matching the ledger), L002 waiting for admin approval ----
    loans_.add(std::move(l1));
    loans_.add(Loan::create("PER", "L002", "CA2001", 150000, 24, LoanStatus::Pending, 0, at(2, 11, 0)));

    // ---- goals, beneficiaries, alerts ----
    goals_.add(std::make_unique<SavingsGoal>("G001", "SB1001", "Laptop", 60000, 22000, util::addDays(now, 120), at(30, 10, 0)));
    goals_.add(std::make_unique<SavingsGoal>("G002", "SB1001", "Emergency Fund", 150000, 45000, util::addDays(now, 300), at(30, 10, 5)));
    payees_.add(std::make_unique<Beneficiary>("SB1001", "Ravi", "CA2001", "Ravi Kumar", at(50, 12, 0)));
    alerts_.add(std::make_unique<Alert>("A001", "CA2001", oddHourRef, "OddHour",
                                        "Debit of " + util::money(9800) + " at 02:40, an unusual hour",
                                        at(22, 2, 40)));

    audit_.log("SYSTEM", "SEED_DATA", "demo accounts, users, transactions, loans and goals created");
    saveAll();
}
