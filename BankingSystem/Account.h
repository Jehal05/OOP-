// Account classes.
//   Account (abstract)
//     |-- SavingsAccount
//     |     `-- StudentAccount          (multilevel inheritance)
//     |-- CurrentAccount
//     `-- FixedDepositAccount
// Each type overrides the virtual functions to follow its own rules (polymorphism).
#pragma once
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

class Account {
protected:                      // derived classes may read these; outside code may not
    std::string accNo_, name_, pinHash_;
    double balance_;
    bool locked_;
    int failedAttempts_;
    time_t opened_;

public:
    Account(const std::string& accNo, const std::string& name, double balance, const std::string& pinHash,
            bool locked = false, int failedAttempts = 0, time_t opened = std::time(nullptr));
    virtual ~Account() = default;

    // rules that differ for each account type
    virtual std::string typeName() const = 0;
    virtual std::string typeCode() const = 0;
    virtual double annualRate() const = 0;                          // interest rate, % per year
    virtual double minBalance() const { return 0; }                 // a negative value means an overdraft limit
    virtual double withdrawalFee(double amount) const { (void)amount; return 0; }
    virtual std::string rulesText() const = 0;

    // getters
    const std::string& accNo() const { return accNo_; }
    const std::string& name() const { return name_; }
    double balance() const { return balance_; }
    bool locked() const { return locked_; }
    time_t opened() const { return opened_; }
    double available() const { return balance_ - minBalance(); }    // how much can be withdrawn
    double monthlyInterest() const { return balance_ * annualRate() / 1200.0; }

    // actions
    void deposit(double amount);
    double withdraw(double amount);                                 // returns the fee that was charged
    bool checkPin(const std::string& pin);                          // wrong PIN 3 times locks the account
    void setPin(const std::string& newPin);
    void unlock();
    void addInterest(double amount) { balance_ += amount; }
    void receive(double amount) { balance_ += amount; }             // money arriving (loan credit, transfer in)

    // file support
    std::string toCsv() const;
    static std::unique_ptr<Account> fromCsv(const std::string& line);
    static std::unique_ptr<Account> create(const std::string& typeCode, const std::string& accNo, const std::string& name,
                                           double balance, const std::string& pinHash, bool locked = false,
                                           int failed = 0, time_t opened = std::time(nullptr));

    friend std::ostream& operator<<(std::ostream& os, const Account& a);   // prints a one-line summary
};

class SavingsAccount : public Account {
public:
    using Account::Account;
    std::string typeName() const override { return "Savings"; }
    std::string typeCode() const override { return "SAV"; }
    double annualRate() const override { return 4.0; }
    double minBalance() const override { return 1000; }
    std::string rulesText() const override { return "4.0% interest, minimum balance Rs 1000"; }
};

class StudentAccount : public SavingsAccount {
public:
    using SavingsAccount::SavingsAccount;
    std::string typeName() const override { return "Student"; }
    std::string typeCode() const override { return "STU"; }
    double annualRate() const override { return 4.5; }
    double minBalance() const override { return 0; }
    std::string rulesText() const override { return "4.5% interest, no minimum balance"; }
};

class CurrentAccount : public Account {
public:
    using Account::Account;
    std::string typeName() const override { return "Current"; }
    std::string typeCode() const override { return "CUR"; }
    double annualRate() const override { return 0.0; }
    double minBalance() const override { return -25000; }           // overdraft up to Rs 25000
    std::string rulesText() const override { return "no interest, overdraft up to Rs 25000"; }
};

class FixedDepositAccount : public Account {
public:
    using Account::Account;
    std::string typeName() const override { return "Fixed Deposit"; }
    std::string typeCode() const override { return "FD"; }
    double annualRate() const override { return 7.0; }
    double withdrawalFee(double amount) const override;             // 2% penalty in the first year
    std::string rulesText() const override { return "7.0% interest, 2% penalty if withdrawn in the first year"; }
};
