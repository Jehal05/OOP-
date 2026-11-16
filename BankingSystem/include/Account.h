// Account.h - abstract Account and its hierarchy.
//   Account (abstract)
//    |- SavingsAccount            single inheritance
//    |   `- StudentSavingsAccount multilevel inheritance
//    |- CurrentAccount
//    `- FixedDepositAccount
#pragma once
#include <string>
#include <memory>
#include <ctime>
#include <ostream>

class Account {
    std::string accNo_, holder_;
    double balance_;
    time_t openedOn_;
    bool active_;
protected:
    void setBalance(double b) { balance_ = b; }
public:
    Account(std::string accNo, std::string holder, double balance, time_t openedOn, bool active = true);
    virtual ~Account() = default;
    // Rule of 5: Account owns no raw resources, so the compiler-generated versions are correct.
    Account(const Account&) = default;
    Account& operator=(const Account&) = default;
    Account(Account&&) noexcept = default;
    Account& operator=(Account&&) noexcept = default;

    // ---- accessors ----
    const std::string& accNo() const { return accNo_; }
    const std::string& holder() const { return holder_; }
    void setHolder(const std::string& h);
    void restoreBalance(double b) { balance_ = b; }          // used only when loading/seeding stored data
    double balance() const { return balance_; }
    time_t openedOn() const { return openedOn_; }
    bool active() const { return active_; }
    // Pays out whatever is left and deactivates the account (used when an admin closes it).
    double closeAndPayout() { double b = balance_; balance_ = 0; active_ = false; return b; }

    // ---- polymorphic behaviour ----
    virtual std::unique_ptr<Account> clone() const = 0;
    virtual std::string typeName() const = 0;
    virtual std::string typeCode() const = 0;                       // used in CSV
    virtual double annualRate() const { return 0.0; }               // % p.a.
    virtual double minimumBalance() const { return 0.0; }
    virtual double spendable() const { return balance_ - minimumBalance(); }
    virtual double withdrawalFee(double amount, time_t now) const { (void)amount; (void)now; return 0.0; }
    virtual double calculateInterest(double years) const = 0;       // interest earned on the current balance
    virtual std::string describe() const = 0;                       // one-line rule summary for the UI
    virtual std::string extraCsv() const = 0;                       // "param1,param2"

    // ---- operations (validate and mutate the balance only; Bank records transactions) ----
    void deposit(double amount);
    void withdraw(double amount, double fee = 0.0);
    Account& operator+=(double amount) { deposit(amount); return *this; }
    bool operator==(const Account& o) const { return accNo_ == o.accNo_; }

    // ---- Repository<T> contract ----
    std::string key() const { return accNo_; }
    std::string toCsv() const;
    static std::unique_ptr<Account> fromCsv(const std::string& line);
    static const char* header() { return "accNo,type,holder,balance,openedOn,active,param1,param2"; }

    friend std::ostream& operator<<(std::ostream& os, const Account& a);
};

class SavingsAccount : public Account {
protected:
    double rate_, minBalance_;
public:
    SavingsAccount(std::string no, std::string holder, double bal, time_t opened, double rate = 4.0,
                   double minBal = 1000.0, bool active = true);
    std::unique_ptr<Account> clone() const override { return std::make_unique<SavingsAccount>(*this); }
    std::string typeName() const override { return "Savings"; }
    std::string typeCode() const override { return "SAV"; }
    double annualRate() const override { return rate_; }
    double minimumBalance() const override { return minBalance_; }
    double calculateInterest(double years) const override;          // simple interest
    std::string describe() const override;
    std::string extraCsv() const override;
};

// Multilevel inheritance: Account -> SavingsAccount -> StudentSavingsAccount
class StudentSavingsAccount : public SavingsAccount {
public:
    StudentSavingsAccount(std::string no, std::string holder, double bal, time_t opened, double rate = 4.5,
                          bool active = true);
    std::unique_ptr<Account> clone() const override { return std::make_unique<StudentSavingsAccount>(*this); }
    std::string typeName() const override { return "Student Savings"; }
    std::string typeCode() const override { return "STU"; }
    std::string describe() const override;
};

class CurrentAccount : public Account {
    double overdraft_;
public:
    CurrentAccount(std::string no, std::string holder, double bal, time_t opened, double overdraft = 25000.0,
                   bool active = true);
    std::unique_ptr<Account> clone() const override { return std::make_unique<CurrentAccount>(*this); }
    std::string typeName() const override { return "Current"; }
    std::string typeCode() const override { return "CUR"; }
    double overdraftLimit() const { return overdraft_; }
    double spendable() const override { return balance() + overdraft_; }
    double calculateInterest(double years) const override { (void)years; return 0.0; }
    std::string describe() const override;
    std::string extraCsv() const override;
};

class FixedDepositAccount : public Account {
    double rate_;
    int lockInMonths_;
public:
    FixedDepositAccount(std::string no, std::string holder, double bal, time_t opened, double rate = 7.0,
                        int lockInMonths = 12, bool active = true);
    std::unique_ptr<Account> clone() const override { return std::make_unique<FixedDepositAccount>(*this); }
    std::string typeName() const override { return "Fixed Deposit"; }
    std::string typeCode() const override { return "FD"; }
    double annualRate() const override { return rate_; }
    int lockInMonths() const { return lockInMonths_; }
    time_t maturityDate() const;
    bool matured(time_t now) const { return now >= maturityDate(); }
    double withdrawalFee(double amount, time_t now) const override;   // 1% penalty before maturity
    double calculateInterest(double years) const override;            // compounded annually
    std::string describe() const override;
    std::string extraCsv() const override;
};
