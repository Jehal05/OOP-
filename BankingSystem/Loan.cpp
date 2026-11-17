#include "Loan.h"
#include "Exceptions.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>

std::string toString(LoanStatus s) {
    switch (s) {
        case LoanStatus::Pending: return "Pending";
        case LoanStatus::Active: return "Active";
        case LoanStatus::Closed: return "Closed";
        case LoanStatus::Rejected: return "Rejected";
    }
    return "?";
}

// EMI = P * r * (1+r)^n / ((1+r)^n - 1)   where r is the monthly rate
double Loan::emi() const {
    double r = annualRate() / 1200.0;
    double growth = std::pow(1 + r, months_);
    return util::round2(principal_ * r * growth / (growth - 1));
}

std::vector<EmiRow> Loan::schedule() const {
    std::vector<EmiRow> rows;
    double r = annualRate() / 1200.0, balance = principal_, installment = emi();
    for (int m = 1; m <= months_; ++m) {
        double interest = util::round2(balance * r);
        double pay = (m == months_) ? util::round2(balance + interest) : installment;   // last one clears rounding
        double principalPart = util::round2(pay - interest);
        balance = util::round2(balance - principalPart);
        rows.push_back({m, pay, interest, principalPart, balance < 0.005 ? 0.0 : balance});
    }
    return rows;
}

double Loan::outstanding() const {
    if (paid_ <= 0) return principal_;
    auto rows = schedule();
    return rows[(size_t)std::min(paid_, months_) - 1].balance;
}

double Loan::nextInstallment() const {
    auto rows = schedule();
    if (paid_ >= months_) return 0;
    return rows[(size_t)paid_].emi;
}

void Loan::payOne() {
    ++paid_;
    if (paid_ >= months_) status_ = LoanStatus::Closed;
}

// ---- file support: typeCode,id,accNo,principal,months,status,paid ----
std::string Loan::toCsv() const {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.2f", principal_);
    return typeCode() + "," + id_ + "," + accNo_ + "," + buf + "," + std::to_string(months_) + "," +
           std::to_string((int)status_) + "," + std::to_string(paid_);
}

std::unique_ptr<Loan> Loan::create(const std::string& code, const std::string& id, const std::string& accNo,
                                   double principal, int months, LoanStatus status, int paid) {
    if (code == "HOME") return std::make_unique<HomeLoan>(id, accNo, principal, months, status, paid);
    if (code == "EDU") return std::make_unique<EducationLoan>(id, accNo, principal, months, status, paid);
    if (code == "PER") return std::make_unique<PersonalLoan>(id, accNo, principal, months, status, paid);
    throw BankException("Unknown loan type: " + code);
}

std::unique_ptr<Loan> Loan::fromCsv(const std::string& line) {
    auto f = util::split(line);
    if (f.size() != 7) throw BankException("Bad loan record: " + line);
    return create(f[0], f[1], f[2], std::atof(f[3].c_str()), std::atoi(f[4].c_str()),
                  (LoanStatus)std::atoi(f[5].c_str()), std::atoi(f[6].c_str()));
}
