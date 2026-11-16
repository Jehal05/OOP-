#include "Loan.h"
#include "Utils.h"
#include <cmath>
#include <cstdio>

std::string toString(LoanStatus s) {
    switch (s) {
        case LoanStatus::Pending: return "Pending";
        case LoanStatus::Active: return "Active";
        case LoanStatus::Rejected: return "Rejected";
        case LoanStatus::Closed: return "Closed";
    }
    return "?";
}

Loan::Loan(std::string id, std::string accNo, double principal, int months, LoanStatus st, int paid,
           time_t applied, std::string note)
    : id_(std::move(id)), accNo_(std::move(accNo)), note_(util::cleanText(note)), principal_(principal),
      months_(months), status_(st), paid_(paid), appliedOn_(applied) {}

void Loan::setNote(const std::string& n) { note_ = util::cleanText(n); }

static double round2(double v) { return std::round(v * 100.0) / 100.0; }

double Loan::emi() const {
    double r = annualRate() / 12.0 / 100.0;
    if (r == 0) return round2(principal_ / months_);
    double f = std::pow(1.0 + r, months_);
    return round2(principal_ * r * f / (f - 1.0));
}

std::vector<Installment> Loan::schedule() const {
    std::vector<Installment> rows;
    double r = annualRate() / 12.0 / 100.0, bal = principal_, e = emi();
    for (int m = 1; m <= months_; ++m) {
        double interest = round2(bal * r);
        double pay = (m == months_) ? round2(bal + interest) : e;     // last EMI clears rounding drift
        double principalPart = round2(pay - interest);
        bal = round2(bal - principalPart);
        if (bal < 0.005) bal = 0;
        rows.push_back({m, pay, interest, principalPart, bal});
    }
    return rows;
}

double Loan::outstanding() const {
    if (status_ != LoanStatus::Active) return status_ == LoanStatus::Pending ? principal_ : 0.0;
    if (paid_ <= 0) return principal_;
    if (paid_ >= months_) return 0.0;
    return schedule()[(size_t)paid_ - 1].balance;
}

double Loan::nextInstallment() const {
    if (paid_ >= months_) return 0.0;
    return schedule()[(size_t)paid_].emi;
}

std::string Loan::toCsv() const {
    char p[32]; std::snprintf(p, sizeof p, "%.2f", principal_);
    return util::join({id_, typeCode(), accNo_, p, std::to_string(months_), std::to_string((int)status_),
                       std::to_string(paid_), util::fmtDateTime(appliedOn_), util::csvEscape(note_)}, ',');
}

std::unique_ptr<Loan> Loan::create(const std::string& code, std::string id, std::string accNo, double principal,
                                   int months, LoanStatus st, int paid, time_t applied, std::string note) {
    if (code == "HOME") return std::make_unique<HomeLoan>(id, accNo, principal, months, st, paid, applied, note);
    if (code == "EDU")  return std::make_unique<EducationLoan>(id, accNo, principal, months, st, paid, applied, note);
    if (code == "PER")  return std::make_unique<PersonalLoan>(id, accNo, principal, months, st, paid, applied, note);
    return nullptr;
}

std::unique_ptr<Loan> Loan::fromCsv(const std::string& line) {
    auto p = util::split(line, ',');
    time_t t;
    if (p.size() < 8 || !util::parseDateTime(p[7], t)) return nullptr;
    return create(p[1], p[0], p[2], std::stod(p[3]), std::stoi(p[4]), (LoanStatus)std::stoi(p[5]), std::stoi(p[6]), t,
                  p.size() > 8 ? util::csvUnescape(p[8]) : "");
}
