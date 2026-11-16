#include "TestHarness.h"
#include "Loan.h"
#include "Utils.h"
#include <cmath>

int main() {
    time_t now = util::now();
    PersonalLoan p("L1", "SB1", 100000, 12, LoanStatus::Active, 0, now);
    // independent evaluation of  P*r*(1+r)^n / ((1+r)^n - 1)
    double r = 0.125 / 12, f = std::pow(1 + r, 12);
    double expected = 100000 * r * f / (f - 1);
    CHECK_NEAR(p.emi(), expected, 0.01);

    auto sch = p.schedule();
    CHECK(sch.size() == 12);
    CHECK_NEAR(sch.back().balance, 0.0, 0.005);                              // fully repaid
    double principalSum = 0;
    for (const auto& row : sch) {
        principalSum += row.principal;
        CHECK_NEAR(row.emi, row.interest + row.principal, 0.011);
    }
    CHECK_NEAR(principalSum, 100000, 0.05);
    CHECK_NEAR(sch.front().interest, 100000 * r, 0.01);                      // first month interest

    // per-type policy via virtual functions
    HomeLoan h("L2", "SB1", 2000000, 240, LoanStatus::Pending, 0, now);
    EducationLoan e("L3", "SB1", 500000, 60, LoanStatus::Pending, 0, now);
    Loan* loans[] = {&h, &e, &p};
    CHECK(loans[0]->annualRate() == 8.5 && loans[1]->annualRate() == 9.5 && loans[2]->annualRate() == 12.5);
    CHECK(loans[0]->maxMonths() == 240 && loans[2]->maxMonths() == 60);

    // outstanding after k installments equals schedule balance
    PersonalLoan q("L4", "SB1", 100000, 12, LoanStatus::Active, 3, now);
    CHECK_NEAR(q.outstanding(), sch[2].balance, 0.005);
    CHECK_NEAR(q.nextInstallment(), sch[3].emi, 0.005);
    CHECK(q.remainingInstallments() == 9);
    PersonalLoan pending("L5", "SB1", 50000, 12, LoanStatus::Pending, 0, now);
    CHECK_NEAR(pending.outstanding(), 50000, 1e-9);

    // CSV round trip keeps the concrete type
    for (Loan* l : loans) {
        auto back = Loan::fromCsv(l->toCsv());
        CHECK(back != nullptr);
        CHECK(back && back->typeCode() == l->typeCode() && back->toCsv() == l->toCsv());
    }
    CHECK(Loan::create("XXX", "", "", 1, 1, LoanStatus::Pending, 0, now) == nullptr);
    return finish("test_loan");
}
