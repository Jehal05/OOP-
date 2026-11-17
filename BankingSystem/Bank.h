// The Bank class holds all the data (accounts, transactions, loans, alerts) and enforces the rules.
// It never prints anything - main.cpp does all the input and output.
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "Account.h"
#include "Fraud.h"
#include "Loan.h"
#include "Transaction.h"

class Bank {
    std::string dir_;                                    // folder for the data files
    std::vector<std::unique_ptr<Account>> accounts_;
    std::vector<Transaction> txns_;
    std::vector<std::unique_ptr<Loan>> loans_;
    std::vector<Alert> alerts_;
    std::vector<std::unique_ptr<FraudRule>> rules_;
    time_t clock_ = 0;                                   // only used to give the demo data old dates

    time_t now() const { return clock_ ? clock_ : std::time(nullptr); }

    std::string path(const std::string& file) const { return dir_ + "/" + file; }
    void saveAccounts() const;
    void saveLoans() const;
    void saveAlerts() const;
    Transaction record(const std::string& accNo, const std::string& type, bool credit, double amount, const std::string& note);
    void screen(const Transaction& tx);                  // run the fraud rules on a debit
    void seedDemoData();

public:
    explicit Bank(const std::string& dir);
    void load();

    // ---- login ----
    Account& login(const std::string& accNo, const std::string& pin);
    bool adminLogin(const std::string& id, const std::string& password);

    // ---- accounts ----
    Account* find(const std::string& accNo);
    Account& account(const std::string& accNo);          // throws NotFoundException
    const std::vector<std::unique_ptr<Account>>& accounts() const { return accounts_; }
    std::string openAccount(const std::string& typeCode, const std::string& name, double initialDeposit, const std::string& pin);
    void changePin(Account& acc, const std::string& oldPin, const std::string& newPin);
    void unlock(const std::string& accNo);
    std::vector<Account*> lockedAccounts();

    // ---- money ----
    Transaction deposit(const std::string& accNo, double amount, const std::string& note);
    Transaction withdraw(const std::string& accNo, double amount, const std::string& note);
    Transaction transfer(const std::string& from, const std::string& to, double amount, const std::string& note);
    double applyMonthlyInterest();                       // returns the total interest paid

    // ---- statements ----
    std::vector<Transaction> history(const std::string& accNo) const;
    std::vector<Transaction> statement(const std::string& accNo, const std::string& type, double minAmount) const;
    std::string exportStatement(const std::string& accNo) const;     // writes a .txt file, returns its path

    // ---- loans ----
    Loan& applyLoan(const std::string& accNo, const std::string& typeCode, double amount, int months);
    std::vector<Loan*> loansOf(const std::string& accNo);
    std::vector<Loan*> pendingLoans();
    void approveLoan(const std::string& id);
    void rejectLoan(const std::string& id);
    Transaction payEmi(const std::string& loanId, const std::string& accNo);

    // ---- fraud alerts ----
    std::vector<Alert*> alertsOf(const std::string& accNo);
    std::vector<Alert*> alertsToReview();
    void markAlertsSeen(const std::string& accNo);
    void reviewAlert(const std::string& id);

    // ---- audit log (append only) ----
    void audit(const std::string& actor, const std::string& action, const std::string& details);
    std::vector<std::string> auditTail(size_t count) const;
};
