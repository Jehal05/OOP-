// Loan.h - abstract Loan with Home / Education / Personal variants (runtime polymorphism).
#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>

enum class LoanStatus { Pending, Active, Rejected, Closed };
std::string toString(LoanStatus s);

struct Installment {
    int month;
    double emi, interest, principal, balance;
};

class Loan {
    std::string id_, accNo_, note_;
    double principal_;
    int months_;
    LoanStatus status_;
    int paid_;
    time_t appliedOn_;
public:
    Loan(std::string id, std::string accNo, double principal, int months, LoanStatus st, int paid,
         time_t applied, std::string note = "");
    virtual ~Loan() = default;

    // ---- polymorphic loan policy ----
    virtual std::string typeName() const = 0;
    virtual std::string typeCode() const = 0;
    virtual double annualRate() const = 0;          // % p.a.
    virtual int maxMonths() const = 0;
    virtual double maxPrincipal() const = 0;

    // ---- shared logic ----
    double emi() const;                              // P*r*(1+r)^n / ((1+r)^n - 1)
    std::vector<Installment> schedule() const;       // full amortization table
    double outstanding() const;                      // principal still owed
    double nextInstallment() const;                  // amount due for the next EMI
    int remainingInstallments() const { return months_ - paid_; }

    const std::string& id() const { return id_; }
    const std::string& accNo() const { return accNo_; }
    const std::string& note() const { return note_; }
    double principal() const { return principal_; }
    int months() const { return months_; }
    LoanStatus status() const { return status_; }
    int paid() const { return paid_; }
    time_t appliedOn() const { return appliedOn_; }
    void setStatus(LoanStatus s) { status_ = s; }
    void setNote(const std::string& n);
    void incrementPaid() { ++paid_; }

    std::string key() const { return id_; }
    std::string toCsv() const;
    static std::unique_ptr<Loan> fromCsv(const std::string& line);
    static std::unique_ptr<Loan> create(const std::string& typeCode, std::string id, std::string accNo,
                                        double principal, int months, LoanStatus st, int paid, time_t applied,
                                        std::string note = "");
    static const char* header() { return "id,type,accNo,principal,months,status,paid,appliedOn,note"; }
};

class HomeLoan : public Loan {
public:
    using Loan::Loan;
    std::string typeName() const override { return "Home Loan"; }
    std::string typeCode() const override { return "HOME"; }
    double annualRate() const override { return 8.5; }
    int maxMonths() const override { return 240; }
    double maxPrincipal() const override { return 5000000.0; }
};
class EducationLoan : public Loan {
public:
    using Loan::Loan;
    std::string typeName() const override { return "Education Loan"; }
    std::string typeCode() const override { return "EDU"; }
    double annualRate() const override { return 9.5; }
    int maxMonths() const override { return 120; }
    double maxPrincipal() const override { return 1500000.0; }
};
class PersonalLoan : public Loan {
public:
    using Loan::Loan;
    std::string typeName() const override { return "Personal Loan"; }
    std::string typeCode() const override { return "PER"; }
    double annualRate() const override { return 12.5; }
    int maxMonths() const override { return 60; }
    double maxPrincipal() const override { return 500000.0; }
};
