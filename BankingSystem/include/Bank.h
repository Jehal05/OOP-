// Bank.h - the controller. Owns every repository and enforces all business rules.
// It never prints anything: the UI layer (Menus) asks the Bank to act and renders the result.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <utility>
#include "Repository.h"
#include "Account.h"
#include "User.h"
#include "Models.h"
#include "Loan.h"
#include "Fraud.h"
#include "AuditLogger.h"
#include "Statement.h"
#include "Exceptions.h"

struct InterestResult { std::string accNo, holder; double amount; std::string status; };
struct SimRow { int year; double simple, compound; };

class Bank {
    std::string dir_;
    Repository<User> users_;
    Repository<Account> accounts_;
    Repository<Transaction> txns_;
    Repository<Beneficiary> payees_;
    Repository<SavingsGoal> goals_;
    Repository<Loan> loans_;
    Repository<Alert> alerts_;
    FraudDetector fraud_;
    AuditLogger audit_;
    std::map<std::string, int> failedOps_;
    int refCounter_ = 0;

    // ---- internals ----
    std::string nextRef(time_t when);
    std::string nextId(const char* prefix, int width, const std::vector<std::string>& existing) const;
    const Transaction& record(Account& a, TxType type, Category cat, double amount, double balanceAfter,
                              const std::string& note, double balanceBefore, bool newPayee, bool screen,
                              time_t when = 0);
    const Transaction& debit(Account& a, TxType type, Category cat, double amount, const std::string& note,
                             bool screen, bool newPayee = false);
    const Transaction& credit(Account& a, TxType type, double amount, const std::string& note, bool screen);
    void screenFraud(const Transaction& tx, double balanceBefore, bool newPayee);
    void seedDemoData();
public:
    explicit Bank(std::string dataDir);
    void load();                       // loads CSVs; seeds demo data on the very first run
    void saveAll() const;
    const std::string& dataDir() const { return dir_; }
    const AuditLogger& audit() const { return audit_; }
    void logAudit(const std::string& actor, const std::string& action, const std::string& details = "") const {
        audit_.log(actor, action, details);
    }

    // ---- authentication & security ----
    User& login(const std::string& id, const std::string& pin, const std::string& role);
    void changePin(User& user, const std::string& oldPin, const std::string& newPin);
    void unlockUser(const std::string& adminId, const std::string& userId);
    std::vector<User*> lockedUsers() const;

    // ---- accounts (admin) ----
    Account& account(const std::string& accNo) const;                 // throws AccountNotFoundException
    Account* findAccount(const std::string& accNo) const { return accounts_.find(accNo); }
    std::vector<Account*> allAccounts() const;
    std::vector<Account*> searchAccounts(const std::string& query) const;
    std::string openAccount(const std::string& adminId, const std::string& typeCode, const std::string& holder,
                            double initialDeposit, const std::string& pin, double p1, double p2);
    void updateHolder(const std::string& adminId, const std::string& accNo, const std::string& newName);
    double closeAccount(const std::string& adminId, const std::string& accNo);

    // ---- money movement ----
    const Transaction& deposit(const std::string& accNo, double amount, const std::string& note = "Cash deposit");
    const Transaction& withdraw(const std::string& accNo, double amount, Category cat, const std::string& note = "");
    const Transaction& transfer(const std::string& from, const std::string& to, double amount, Category cat,
                                const std::string& note = "");
    std::vector<Transaction> history(const std::string& accNo) const;
    const Transaction* findTransaction(const std::string& refId) const;
    double fdWithdrawalFee(const std::string& accNo, double amount) const;

    // ---- beneficiaries ----
    const Beneficiary& addPayee(const std::string& owner, const std::string& nickname, const std::string& payeeAcc);
    void removePayee(const std::string& owner, const std::string& nickname);
    std::vector<Beneficiary*> payeesOf(const std::string& owner) const;
    const Beneficiary* findPayee(const std::string& owner, const std::string& nickname) const;

    // ---- savings goals ----
    const SavingsGoal& createGoal(const std::string& accNo, const std::string& name, double target, time_t deadline);
    const Transaction& contributeToGoal(const std::string& goalId, double amount);
    std::vector<SavingsGoal*> goalsOf(const std::string& accNo) const;

    // ---- loans ----
    const Loan& applyLoan(const std::string& accNo, const std::string& typeCode, double principal, int months);
    void approveLoan(const std::string& adminId, const std::string& loanId);
    void rejectLoan(const std::string& adminId, const std::string& loanId, const std::string& reason);
    const Transaction& payEMI(const std::string& loanId);
    const Transaction& prepayLoan(const std::string& loanId);
    std::vector<Loan*> loansOf(const std::string& accNo) const;
    std::vector<Loan*> pendingLoans() const;
    Loan* findLoan(const std::string& id) const { return loans_.find(id); }

    // ---- fraud alerts ----
    std::vector<Alert*> unseenAlerts(const std::string& accNo) const;
    std::vector<Alert*> alertsOf(const std::string& accNo) const;
    void markAlertsSeen(const std::string& accNo);
    std::vector<Alert*> alertsAwaitingReview() const;
    void markAlertReviewed(const std::string& adminId, const std::string& alertId);
    size_t fraudRuleCount() const { return fraud_.ruleCount(); }

    // ---- interest, simulator, reports ----
    std::vector<InterestResult> applyMonthlyInterest(const std::string& adminId);
    static std::vector<SimRow> simulate(const Account& a, double principal, int years);
    double totalDeposits() const;
    double totalBalance() const;
    std::vector<Account*> richest(size_t n) const;
    std::vector<std::pair<Account*, time_t>> dormant(int days) const;
    std::map<std::string, std::pair<double, double>> periodTotals(bool monthly) const;   // key -> (credits, debits)
};
