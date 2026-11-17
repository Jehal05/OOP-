// Loans. Loan is an abstract class; each loan type has its own rate and limits.
#pragma once
#include <memory>
#include <string>
#include <vector>

enum class LoanStatus { Pending, Active, Closed, Rejected };
std::string toString(LoanStatus s);

struct EmiRow {                 // one line of the amortization table
    int month;
    double emi, interest, principal, balance;
};

class Loan {
protected:
    std::string id_, accNo_;
    double principal_;
    int months_;
    LoanStatus status_;
    int paid_;                  // installments paid so far

public:
    Loan(const std::string& id, const std::string& accNo, double principal, int months, LoanStatus status = LoanStatus::Pending, int paid = 0)
        : id_(id), accNo_(accNo), principal_(principal), months_(months), status_(status), paid_(paid) {}
    virtual ~Loan() = default;

    virtual std::string typeName() const = 0;
    virtual std::string typeCode() const = 0;
    virtual double annualRate() const = 0;      // % per year
    virtual double maxAmount() const = 0;
    virtual int maxMonths() const = 0;

    const std::string& id() const { return id_; }
    const std::string& accNo() const { return accNo_; }
    double principal() const { return principal_; }
    int months() const { return months_; }
    LoanStatus status() const { return status_; }
    int paid() const { return paid_; }
    void setStatus(LoanStatus s) { status_ = s; }
    void payOne();                               // counts one installment; closes the loan after the last one

    double emi() const;                          // monthly installment
    std::vector<EmiRow> schedule() const;        // full amortization table
    double outstanding() const;                  // principal still to be repaid
    double nextInstallment() const;              // amount due now (the last one absorbs rounding)

    std::string toCsv() const;
    static std::unique_ptr<Loan> create(const std::string& typeCode, const std::string& id, const std::string& accNo,
                                        double principal, int months, LoanStatus status = LoanStatus::Pending, int paid = 0);
    static std::unique_ptr<Loan> fromCsv(const std::string& line);
};

class HomeLoan : public Loan {
public:
    using Loan::Loan;
    std::string typeName() const override { return "Home loan"; }
    std::string typeCode() const override { return "HOME"; }
    double annualRate() const override { return 8.5; }
    double maxAmount() const override { return 5000000; }
    int maxMonths() const override { return 240; }
};

class EducationLoan : public Loan {
public:
    using Loan::Loan;
    std::string typeName() const override { return "Education loan"; }
    std::string typeCode() const override { return "EDU"; }
    double annualRate() const override { return 9.5; }
    double maxAmount() const override { return 1500000; }
    int maxMonths() const override { return 120; }
};

class PersonalLoan : public Loan {
public:
    using Loan::Loan;
    std::string typeName() const override { return "Personal loan"; }
    std::string typeCode() const override { return "PER"; }
    double annualRate() const override { return 12.5; }
    double maxAmount() const override { return 500000; }
    int maxMonths() const override { return 60; }
};
