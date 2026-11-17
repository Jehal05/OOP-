#include "Account.h"
#include "Exceptions.h"
#include "Utils.h"
#include <cstdio>
#include <cstdlib>

Account::Account(const std::string& accNo, const std::string& name, double balance, const std::string& pinHash,
                 bool locked, int failedAttempts, time_t opened)
    : accNo_(accNo), name_(name), pinHash_(pinHash), balance_(balance), locked_(locked),
      failedAttempts_(failedAttempts), opened_(opened) {}

void Account::deposit(double amount) {
    if (amount <= 0) throw InvalidInputException("Amount must be greater than zero.");
    balance_ += amount;
}

double Account::withdraw(double amount) {
    if (amount <= 0) throw InvalidInputException("Amount must be greater than zero.");
    double fee = withdrawalFee(amount);
    if (amount + fee > available()) throw InsufficientFundsException();
    balance_ -= amount + fee;
    return fee;
}

bool Account::checkPin(const std::string& pin) {
    if (locked_) throw AccountLockedException();
    if (util::hashPin(pin, accNo_) == pinHash_) {
        failedAttempts_ = 0;
        return true;
    }
    if (++failedAttempts_ >= 3) locked_ = true;
    return false;
}

void Account::setPin(const std::string& newPin) { pinHash_ = util::hashPin(newPin, accNo_); }

void Account::unlock() { locked_ = false; failedAttempts_ = 0; }

double FixedDepositAccount::withdrawalFee(double amount) const {
    bool inFirstYear = std::time(nullptr) - opened_ < 365LL * 24 * 3600;
    return inFirstYear ? util::round2(amount * 0.02) : 0.0;
}

// ---- file support: typeCode,accNo,name,balance,pinHash,locked,failedAttempts,opened ----
std::string Account::toCsv() const {
    char bal[64];
    std::snprintf(bal, sizeof bal, "%.2f", balance_);
    return typeCode() + "," + accNo_ + "," + name_ + "," + bal + "," + pinHash_ + "," + (locked_ ? "1" : "0") + "," +
           std::to_string(failedAttempts_) + "," + std::to_string((long long)opened_);
}

std::unique_ptr<Account> Account::create(const std::string& code, const std::string& accNo, const std::string& name,
                                         double balance, const std::string& pinHash, bool locked, int failed,
                                         time_t opened) {
    if (code == "SAV") return std::make_unique<SavingsAccount>(accNo, name, balance, pinHash, locked, failed, opened);
    if (code == "STU") return std::make_unique<StudentAccount>(accNo, name, balance, pinHash, locked, failed, opened);
    if (code == "CUR") return std::make_unique<CurrentAccount>(accNo, name, balance, pinHash, locked, failed, opened);
    if (code == "FD") return std::make_unique<FixedDepositAccount>(accNo, name, balance, pinHash, locked, failed, opened);
    throw BankException("Unknown account type: " + code);
}

std::unique_ptr<Account> Account::fromCsv(const std::string& line) {
    auto f = util::split(line);
    if (f.size() != 8) throw BankException("Bad account record: " + line);
    return create(f[0], f[1], f[2], std::atof(f[3].c_str()), f[4], f[5] == "1", std::atoi(f[6].c_str()),
                  (time_t)std::atoll(f[7].c_str()));
}

std::ostream& operator<<(std::ostream& os, const Account& a) {
    os << a.accNo_ << "  " << a.name_ << "  (" << a.typeName() << ")  " << util::money(a.balance_);
    return os;
}
