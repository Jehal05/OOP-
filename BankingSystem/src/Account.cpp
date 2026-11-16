#include "Account.h"
#include "Exceptions.h"
#include "Utils.h"
#include <cmath>
#include <cstdio>

Account::Account(std::string accNo, std::string holder, double balance, time_t openedOn, bool active)
    : accNo_(std::move(accNo)), holder_(util::sanitize(holder)), balance_(balance), openedOn_(openedOn), active_(active) {}

void Account::setHolder(const std::string& h) { holder_ = util::sanitize(h); }

void Account::deposit(double amount) {
    if (!(amount > 0)) throw InvalidAmountException("amount must be greater than zero");
    if (!active_) throw BankException("Account is closed.");
    balance_ += amount;
}

void Account::withdraw(double amount, double fee) {
    if (!(amount > 0)) throw InvalidAmountException("amount must be greater than zero");
    if (!active_) throw BankException("Account is closed.");
    double total = amount + fee;
    if (total > spendable() + 1e-9)
        throw InsufficientFundsException(spendable(), total,
            "Available to withdraw: " + util::money(std::max(0.0, spendable())) + ", required: " + util::money(total) + ".");
    balance_ -= total;
}

std::string Account::toCsv() const {
    return util::join({accNo_, typeCode(), holder_, util::fixedWidth(balance_), util::fmtDateTime(openedOn_),
                       active_ ? "1" : "0", extraCsv()}, ',');
}

std::unique_ptr<Account> Account::fromCsv(const std::string& line) {
    auto p = util::split(line, ',');
    time_t opened;
    if (p.size() < 8 || !util::parseDateTime(p[4], opened)) return nullptr;
    double bal = std::stod(p[3]), p1 = std::stod(p[6]), p2 = std::stod(p[7]);
    bool active = p[5] == "1";
    const std::string& t = p[1];
    if (t == "SAV") return std::make_unique<SavingsAccount>(p[0], p[2], bal, opened, p1, p2, active);
    if (t == "STU") return std::make_unique<StudentSavingsAccount>(p[0], p[2], bal, opened, p1, active);
    if (t == "CUR") return std::make_unique<CurrentAccount>(p[0], p[2], bal, opened, p1, active);
    if (t == "FD")  return std::make_unique<FixedDepositAccount>(p[0], p[2], bal, opened, p1, (int)p2, active);
    return nullptr;
}

std::ostream& operator<<(std::ostream& os, const Account& a) {
    return os << a.accNo_ << " | " << a.typeName() << " | " << a.holder_ << " | " << util::money(a.balance_);
}

static std::string two(double v) { char b[32]; std::snprintf(b, sizeof b, "%.2f", v); return b; }

// ---------------------------------------------------------------- Savings
SavingsAccount::SavingsAccount(std::string no, std::string holder, double bal, time_t opened, double rate,
                               double minBal, bool active)
    : Account(std::move(no), std::move(holder), bal, opened, active), rate_(rate), minBalance_(minBal) {}
double SavingsAccount::calculateInterest(double years) const { return balance() * rate_ * years / 100.0; }
std::string SavingsAccount::describe() const {
    return two(rate_) + "% p.a. simple interest | minimum balance " + util::money(minBalance_);
}
std::string SavingsAccount::extraCsv() const { return two(rate_) + "," + two(minBalance_); }

StudentSavingsAccount::StudentSavingsAccount(std::string no, std::string holder, double bal, time_t opened,
                                             double rate, bool active)
    : SavingsAccount(std::move(no), std::move(holder), bal, opened, rate, 0.0, active) {}
std::string StudentSavingsAccount::describe() const {
    return two(rate_) + "% p.a. simple interest | zero minimum balance (student)";
}

// ---------------------------------------------------------------- Current
CurrentAccount::CurrentAccount(std::string no, std::string holder, double bal, time_t opened, double od, bool active)
    : Account(std::move(no), std::move(holder), bal, opened, active), overdraft_(od) {}
std::string CurrentAccount::describe() const { return "No interest | overdraft limit " + util::money(overdraft_); }
std::string CurrentAccount::extraCsv() const { return two(overdraft_) + ",0.00"; }

// ---------------------------------------------------------------- Fixed deposit
FixedDepositAccount::FixedDepositAccount(std::string no, std::string holder, double bal, time_t opened, double rate,
                                         int lockIn, bool active)
    : Account(std::move(no), std::move(holder), bal, opened, active), rate_(rate), lockInMonths_(lockIn) {}
time_t FixedDepositAccount::maturityDate() const { return util::addMonths(openedOn(), lockInMonths_); }
double FixedDepositAccount::withdrawalFee(double amount, time_t now) const {
    return matured(now) ? 0.0 : std::round(amount * 1.0) / 100.0;      // 1% penalty, rounded to paise
}
double FixedDepositAccount::calculateInterest(double years) const {
    return balance() * (std::pow(1.0 + rate_ / 100.0, years) - 1.0);
}
std::string FixedDepositAccount::describe() const {
    return two(rate_) + "% p.a. compounded yearly | lock-in " + std::to_string(lockInMonths_) +
           " months | matures " + util::fmtDate(maturityDate()) + " | 1% early-withdrawal penalty";
}
std::string FixedDepositAccount::extraCsv() const { return two(rate_) + "," + std::to_string(lockInMonths_); }
