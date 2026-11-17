#include "Bank.h"
#include "Exceptions.h"
#include "Utils.h"
#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static const std::string ADMIN_ID = "admin";

Bank::Bank(const std::string& dir) : dir_(dir) {
    rules_.push_back(std::make_unique<LargeAmountRule>());
    rules_.push_back(std::make_unique<RapidTransactionsRule>());
    rules_.push_back(std::make_unique<OddHourRule>());
}

// ======================================================= loading and saving
void Bank::load() {
    fs::create_directories(dir_);
    if (!fs::exists(path("accounts.csv"))) {
        seedDemoData();
        return;
    }
    std::string line;
    std::ifstream fa(path("accounts.csv"));
    while (std::getline(fa, line)) if (!line.empty()) accounts_.push_back(Account::fromCsv(line));
    std::ifstream ft(path("transactions.csv"));
    while (std::getline(ft, line)) if (!line.empty()) txns_.push_back(Transaction::fromCsv(line));
    std::ifstream fl(path("loans.csv"));
    while (std::getline(fl, line)) if (!line.empty()) loans_.push_back(Loan::fromCsv(line));
    std::ifstream fz(path("alerts.csv"));
    while (std::getline(fz, line)) if (!line.empty()) alerts_.push_back(Alert::fromCsv(line));
}

void Bank::saveAccounts() const {
    std::ofstream out(path("accounts.csv"));
    for (const auto& a : accounts_) out << a->toCsv() << "\n";
}
void Bank::saveLoans() const {
    std::ofstream out(path("loans.csv"));
    for (const auto& l : loans_) out << l->toCsv() << "\n";
}
void Bank::saveAlerts() const {
    std::ofstream out(path("alerts.csv"));
    for (const auto& a : alerts_) out << a.toCsv() << "\n";
}

// midday, a number of days ago (so the demo history never looks like an odd-hour transaction)
static time_t daysAgo(int days) {
    std::time_t t = std::time(nullptr) - (std::time_t)days * 24 * 3600;
    std::tm* tmv = std::localtime(&t);
    tmv->tm_hour = 12; tmv->tm_min = 0; tmv->tm_sec = 0;
    return std::mktime(tmv);
}

void Bank::seedDemoData() {
    auto add = [&](const char* code, const char* no, const char* name, double bal, const char* pin, int daysOld) {
        accounts_.push_back(Account::create(code, no, name, 0, util::hashPin(pin, no), false, 0, daysAgo(daysOld)));
        clock_ = daysAgo(daysOld);
        deposit(no, bal, "Opening balance");
    };
    add("SAV", "SB1001", "Asha Verma", 150000, "1234", 200);
    add("CUR", "CA2001", "Ravi Kumar", 60000, "2345", 150);
    add("FD", "FD3001", "Meera Nair", 200000, "3456", 100);
    add("STU", "ST1001", "Arjun Rao", 15000, "4567", 60);
    clock_ = daysAgo(30); withdraw("SB1001", 15000, "House rent");
    clock_ = daysAgo(12); withdraw("SB1001", 3200, "Electricity bill");
    clock_ = daysAgo(5);  transfer("SB1001", "CA2001", 5000, "Shared expenses");
    clock_ = 0;
    // one active loan, one waiting for approval, and one old alert
    loans_.push_back(Loan::create("PER", "L001", "SB1001", 100000, 12, LoanStatus::Active, 3));
    loans_.push_back(Loan::create("EDU", "L002", "CA2001", 250000, 60, LoanStatus::Pending, 0));
    alerts_.push_back(Alert{"A001", "CA2001", "Odd hour", "Debit of Rs 9800.00 at an unusual hour (demo alert)", daysAgo(20), false, false});
    saveLoans();
    saveAccounts();
    saveAlerts();
    audit("system", "SEED", "demo data created");
}

// ======================================================= login
Account* Bank::find(const std::string& accNo) {
    std::string key = util::upper(util::trim(accNo));
    for (auto& a : accounts_) if (a->accNo() == key) return a.get();
    return nullptr;
}

Account& Bank::account(const std::string& accNo) {
    Account* a = find(accNo);
    if (!a) throw NotFoundException("Account " + accNo);
    return *a;
}

Account& Bank::login(const std::string& accNo, const std::string& pin) {
    Account* a = find(accNo);
    if (!a) throw NotFoundException("Account " + accNo);
    bool ok = false;
    try {
        ok = a->checkPin(pin);
    } catch (const AccountLockedException&) {
        audit(a->accNo(), "LOGIN_BLOCKED", "account is locked");
        throw;
    }
    saveAccounts();                                       // remember failed attempts
    if (!ok) {
        audit(a->accNo(), "LOGIN_FAILED", "wrong PIN");
        if (a->locked()) {
            audit(a->accNo(), "ACCOUNT_LOCKED", "3 wrong PINs");
            throw AccountLockedException();
        }
        throw BankException("Wrong PIN.");
    }
    audit(a->accNo(), "LOGIN", "customer login");
    return *a;
}

bool Bank::adminLogin(const std::string& id, const std::string& password) {
    bool ok = id == ADMIN_ID && util::hashPin(password, ADMIN_ID) == util::hashPin("admin123", ADMIN_ID);
    audit(id, ok ? "ADMIN_LOGIN" : "ADMIN_LOGIN_FAILED", "");
    return ok;
}

// ======================================================= accounts
std::string Bank::openAccount(const std::string& code, const std::string& name, double initial, const std::string& pin) {
    std::string cleanName = util::clean(name);
    if (cleanName.empty()) throw InvalidInputException("Name cannot be empty.");
    if (pin.size() != 4 || !std::all_of(pin.begin(), pin.end(), ::isdigit)) throw InvalidInputException("PIN must be exactly 4 digits.");
    if (initial < 0) throw InvalidInputException("Opening deposit cannot be negative.");
    // new account number: prefix by type + next free number
    std::string prefix = code == "SAV" ? "SB" : code == "STU" ? "ST" : code == "CUR" ? "CA" : "FD";
    int next = 1000;
    for (const auto& a : accounts_)
        if (a->accNo().rfind(prefix, 0) == 0) next = std::max(next, std::atoi(a->accNo().c_str() + 2));
    std::string accNo = prefix + std::to_string(next + 1);
    auto acc = Account::create(code, accNo, cleanName, 0, util::hashPin(pin, accNo));
    if (acc->minBalance() > 0 && initial < acc->minBalance())
        throw InvalidInputException("Minimum opening deposit for this account is " + util::money(acc->minBalance()) + ".");
    accounts_.push_back(std::move(acc));
    if (initial > 0) deposit(accNo, initial, "Opening deposit");
    saveAccounts();
    audit(ADMIN_ID, "ACCOUNT_OPENED", accNo + " for " + cleanName);
    return accNo;
}

void Bank::changePin(Account& acc, const std::string& oldPin, const std::string& newPin) {
    if (!acc.checkPin(oldPin)) { saveAccounts(); throw BankException("Current PIN is wrong."); }
    if (newPin.size() != 4 || !std::all_of(newPin.begin(), newPin.end(), ::isdigit)) throw InvalidInputException("PIN must be exactly 4 digits.");
    acc.setPin(newPin);
    saveAccounts();
    audit(acc.accNo(), "PIN_CHANGED", "");
}

void Bank::unlock(const std::string& accNo) {
    account(accNo).unlock();
    saveAccounts();
    audit(ADMIN_ID, "ACCOUNT_UNLOCKED", accNo);
}

std::vector<Account*> Bank::lockedAccounts() {
    std::vector<Account*> out;
    for (auto& a : accounts_) if (a->locked()) out.push_back(a.get());
    return out;
}

// ======================================================= money
Transaction Bank::record(const std::string& accNo, const std::string& type, bool credit, double amount, const std::string& note) {
    std::time_t when = now();
    char ref[40];
    std::snprintf(ref, sizeof ref, "TXN-%s-%06d", util::compactDate(when).c_str(), (int)txns_.size() + 1);   // TXN-20261116-000001
    Transaction tx{ref, accNo, type, credit, amount, account(accNo).balance(), when, util::clean(note)};
    txns_.push_back(tx);
    std::ofstream out(path("transactions.csv"), std::ios::app);
    out << tx.toCsv() << "\n";
    return tx;
}

void Bank::screen(const Transaction& tx) {
    auto hist = history(tx.accNo);
    for (const auto& rule : rules_) {
        std::string msg;
        if (rule->check(tx, hist, msg)) {
            char id[16];
            std::snprintf(id, sizeof id, "A%03d", (int)alerts_.size() + 1);
            alerts_.push_back(Alert{id, tx.accNo, rule->name(), msg + " [" + tx.ref + "]", tx.time, false, false});
            audit(tx.accNo, "FRAUD_ALERT", rule->name() + " " + tx.ref);
        }
    }
    saveAlerts();
}

Transaction Bank::deposit(const std::string& accNo, double amount, const std::string& note) {
    Account& a = account(accNo);
    a.deposit(amount);
    saveAccounts();
    Transaction tx = record(accNo, "Deposit", true, amount, note);
    audit(accNo, "DEPOSIT", tx.ref + " " + util::money(amount));
    return tx;
}

Transaction Bank::withdraw(const std::string& accNo, double amount, const std::string& note) {
    Account& a = account(accNo);
    double fee = a.withdraw(amount);
    saveAccounts();
    Transaction tx = record(accNo, "Withdraw", false, amount, note);
    if (fee > 0) record(accNo, "Fee", false, fee, "Early withdrawal penalty");
    audit(accNo, "WITHDRAW", tx.ref + " " + util::money(amount));
    screen(tx);
    return tx;
}

Transaction Bank::transfer(const std::string& from, const std::string& to, double amount, const std::string& note) {
    Account& src = account(from);
    Account* dst = find(to);
    if (!dst) throw NotFoundException("Recipient account " + to);
    if (dst->accNo() == src.accNo()) throw InvalidInputException("You cannot transfer to your own account.");
    double fee = src.withdraw(amount);
    dst->receive(amount);
    saveAccounts();
    Transaction out = record(src.accNo(), "Transfer Out", false, amount, note.empty() ? "To " + dst->accNo() : note);
    record(dst->accNo(), "Transfer In", true, amount, note.empty() ? "From " + src.accNo() : note);
    if (fee > 0) record(src.accNo(), "Fee", false, fee, "Early withdrawal penalty");
    audit(src.accNo(), "TRANSFER", out.ref + " " + util::money(amount) + " to " + dst->accNo());
    screen(out);
    return out;
}

double Bank::applyMonthlyInterest() {
    double total = 0;
    for (auto& a : accounts_) {
        double interest = util::round2(a->monthlyInterest());
        if (interest <= 0) continue;
        a->addInterest(interest);
        record(a->accNo(), "Interest", true, interest, "Monthly interest");
        total += interest;
    }
    saveAccounts();
    audit(ADMIN_ID, "INTEREST_APPLIED", util::money(total));
    return total;
}

// ======================================================= statements
std::vector<Transaction> Bank::history(const std::string& accNo) const {
    std::vector<Transaction> out;
    for (const auto& t : txns_) if (t.accNo == accNo) out.push_back(t);
    return out;
}

std::vector<Transaction> Bank::statement(const std::string& accNo, const std::string& type, double minAmount) const {
    std::vector<Transaction> out;
    for (const auto& t : history(accNo))
        if ((type.empty() || util::lower(t.type) == util::lower(type)) && t.amount >= minAmount) out.push_back(t);
    return out;
}

std::string Bank::exportStatement(const std::string& accNo) const {
    std::string file = path("statement_" + accNo + ".txt");
    std::ofstream out(file);
    out << "ACCOUNT STATEMENT - " << accNo << "\n\n";
    for (const auto& t : history(accNo))
        out << util::formatTime(t.time) << "  " << t.ref << "  " << t.type << "  " << (t.credit ? "+" : "-")
            << util::money(t.amount) << "  balance " << util::money(t.balanceAfter) << "  " << t.note << "\n";
    return file;
}

// ======================================================= loans
Loan& Bank::applyLoan(const std::string& accNo, const std::string& code, double amount, int months) {
    char id[16];
    std::snprintf(id, sizeof id, "L%03d", (int)loans_.size() + 1);
    auto loan = Loan::create(code, id, accNo, amount, months);
    if (amount <= 0 || amount > loan->maxAmount())
        throw InvalidInputException("A " + loan->typeName() + " must be between 1 and " + util::money(loan->maxAmount()) + ".");
    if (months < 1 || months > loan->maxMonths())
        throw InvalidInputException("Tenure must be between 1 and " + std::to_string(loan->maxMonths()) + " months.");
    loans_.push_back(std::move(loan));
    saveLoans();
    audit(accNo, "LOAN_APPLIED", std::string(id) + " " + util::money(amount));
    return *loans_.back();
}

std::vector<Loan*> Bank::loansOf(const std::string& accNo) {
    std::vector<Loan*> out;
    for (auto& l : loans_) if (l->accNo() == accNo) out.push_back(l.get());
    return out;
}

std::vector<Loan*> Bank::pendingLoans() {
    std::vector<Loan*> out;
    for (auto& l : loans_) if (l->status() == LoanStatus::Pending) out.push_back(l.get());
    return out;
}

static Loan* findLoan(std::vector<std::unique_ptr<Loan>>& loans, const std::string& id) {
    std::string key = util::upper(util::trim(id));
    for (auto& l : loans) if (l->id() == key) return l.get();
    throw NotFoundException("Loan " + id);
}

void Bank::approveLoan(const std::string& id) {
    Loan* l = findLoan(loans_, id);
    if (l->status() != LoanStatus::Pending) throw BankException("Only pending loans can be approved.");
    l->setStatus(LoanStatus::Active);
    account(l->accNo()).receive(l->principal());
    saveAccounts();
    record(l->accNo(), "Loan Credit", true, l->principal(), "Loan " + l->id() + " approved");
    saveLoans();
    audit(ADMIN_ID, "LOAN_APPROVED", l->id());
}

void Bank::rejectLoan(const std::string& id) {
    Loan* l = findLoan(loans_, id);
    if (l->status() != LoanStatus::Pending) throw BankException("Only pending loans can be rejected.");
    l->setStatus(LoanStatus::Rejected);
    saveLoans();
    audit(ADMIN_ID, "LOAN_REJECTED", l->id());
}

Transaction Bank::payEmi(const std::string& loanId, const std::string& accNo) {
    Loan* l = findLoan(loans_, loanId);
    if (l->accNo() != accNo) throw NotFoundException("Loan " + loanId);
    if (l->status() != LoanStatus::Active) throw BankException("This loan is not active.");
    double due = l->nextInstallment();
    account(accNo).withdraw(due);
    l->payOne();
    saveAccounts();
    saveLoans();
    Transaction tx = record(accNo, "EMI", false, due, "EMI for loan " + l->id());
    audit(accNo, "EMI_PAID", l->id() + " " + util::money(due));
    return tx;
}

// ======================================================= fraud alerts
std::vector<Alert*> Bank::alertsOf(const std::string& accNo) {
    std::vector<Alert*> out;
    for (auto& a : alerts_) if (a.accNo == accNo) out.push_back(&a);
    return out;
}

std::vector<Alert*> Bank::alertsToReview() {
    std::vector<Alert*> out;
    for (auto& a : alerts_) if (!a.reviewed) out.push_back(&a);
    return out;
}

void Bank::markAlertsSeen(const std::string& accNo) {
    for (auto& a : alerts_) if (a.accNo == accNo) a.seen = true;
    saveAlerts();
}

void Bank::reviewAlert(const std::string& id) {
    std::string key = util::upper(util::trim(id));
    for (auto& a : alerts_)
        if (a.id == key) { a.reviewed = true; saveAlerts(); audit(ADMIN_ID, "ALERT_REVIEWED", key); return; }
    throw NotFoundException("Alert " + id);
}

// ======================================================= audit log
void Bank::audit(const std::string& actor, const std::string& action, const std::string& details) {
    fs::create_directories(dir_);
    std::ofstream out(path("audit.log"), std::ios::app);          // append only: old lines are never changed
    out << util::formatTime(now()) << " | " << actor << " | " << action << " | " << details << "\n";
}

std::vector<std::string> Bank::auditTail(size_t count) const {
    std::vector<std::string> lines;
    std::ifstream in(path("audit.log"));
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    if (lines.size() > count) lines.erase(lines.begin(), lines.end() - (long)count);
    return lines;
}
