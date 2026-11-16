// App.h - the presentation layer: every screen, menu and prompt of the application.
#pragma once
#include <string>
#include <vector>
#include <chrono>
#include "Bank.h"
#include "ConsoleUI.h"

struct SessionExpired {};

// Prints "[1] Food  [2] Bills ..." wrapped at perLine entries so it fits 80 columns.
void printChoices(const std::string& label, const std::vector<std::string>& names, size_t perLine);

struct MenuItem {
    int key;
    std::string label;
    std::string desc;
};

class App {
    Bank& bank_;
    std::string stmtDir_, receiptDir_;
    std::chrono::steady_clock::time_point lastActivity_;
    int timeoutSec_;

    // ---- shared helpers ----
    void touch() { lastActivity_ = std::chrono::steady_clock::now(); }
    void checkSession() const;
    int choose(const std::vector<MenuItem>& items, const std::string& prompt = "Choose an option");
    void showError(const BankException& e);
    Category pickCategory();
    std::string amountCell(const Transaction& t) const;
    void receipt(const Transaction& tx, bool save = true);
    void accountCard(const Account& a);
    bool askDate(const std::string& prompt, std::optional<time_t>& out);
    StatementFilter askFilter();
    void showStatementTable(const std::vector<Transaction>& rows, size_t limit = 20);
    void statementFlow(const std::string& accNo, const std::string& crumb);
    void loanTable(const std::vector<Loan*>& loans, bool withAccount = false);
    void amortizationTable(const Loan& loan);
    void emiPreview(const Loan& probe);

    // ---- customer ----
    void customerSession(User& user);
    void customerDashboard(const Customer& c);
    void transactionsMenu(const std::string& acc);
    void doDeposit(const std::string& acc);
    void doWithdraw(const std::string& acc);
    void doTransfer(const std::string& acc);
    void statementMenu(const std::string& acc);
    void spendingReport(const std::string& acc);
    void goalsMenu(const std::string& acc);
    void loansMenu(const std::string& acc);
    void payeesMenu(const std::string& acc);
    void simulatorMenu(const std::string& acc);
    void securityMenu(Customer& c);

    // ---- admin ----
    void adminSession(User& admin);
    void adminAccounts(const std::string& adminId);
    void adminOpenAccount(const std::string& adminId);
    void adminLoans(const std::string& adminId);
    void adminAlerts(const std::string& adminId);
    void adminReports();
    void adminSecurity(const std::string& adminId);
    void adminInterest(const std::string& adminId);
public:
    explicit App(Bank& bank);
    void run();
};
