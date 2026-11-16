#include "TestHarness.h"
#include "Account.h"
#include "Exceptions.h"
#include "Utils.h"

int main() {
    time_t now = util::now();

    // --- savings: minimum balance is respected ---
    SavingsAccount s("SB9", "Test", 5000, now, 4.0, 1000);
    s.deposit(1000);
    CHECK_NEAR(s.balance(), 6000, 1e-9);
    s.withdraw(5000);                                   // leaves exactly the minimum
    CHECK_NEAR(s.balance(), 1000, 1e-9);
    CHECK_THROWS(s.withdraw(1), InsufficientFundsException);
    CHECK_THROWS(s.deposit(0), InvalidAmountException);
    CHECK_THROWS(s.deposit(-5), InvalidAmountException);
    s += 500;                                           // operator+=
    CHECK_NEAR(s.balance(), 1500, 1e-9);

    // --- polymorphic interest ---
    SavingsAccount s2("S2", "T", 10000, now, 4.0);
    CurrentAccount c("CA9", "Test", 1000, now, 5000);
    FixedDepositAccount fd("FD9", "Test", 10000, now, 7.0, 12);
    Account* all[] = {&s2, &c, &fd};
    CHECK_NEAR(all[0]->calculateInterest(2), 800.0, 1e-6);                // simple
    CHECK_NEAR(all[1]->calculateInterest(2), 0.0, 1e-9);                  // none
    CHECK_NEAR(all[2]->calculateInterest(2), 10000 * (1.07 * 1.07 - 1), 1e-6);   // compound

    // --- current: overdraft ---
    c.withdraw(5000);
    CHECK_NEAR(c.balance(), -4000, 1e-9);
    CHECK_THROWS(c.withdraw(1500), InsufficientFundsException);           // beyond overdraft
    CHECK(c.spendable() == 1000);

    // --- FD: penalty before maturity only ---
    CHECK_NEAR(fd.withdrawalFee(1000, now), 10.0, 1e-9);
    CHECK_NEAR(fd.withdrawalFee(1000, util::addMonths(now, 13)), 0.0, 1e-9);
    CHECK(!fd.matured(now));

    // --- multilevel inheritance ---
    StudentSavingsAccount st("ST9", "Stu", 0, now);
    CHECK(st.typeName() == "Student Savings");
    st.deposit(10);
    st.withdraw(10);                                                      // zero minimum balance
    CHECK_NEAR(st.balance(), 0, 1e-9);

    // --- clone is a deep, independent copy ---
    auto copy = s2.clone();
    copy->deposit(1);
    CHECK_NEAR(s2.balance(), 10000, 1e-9);
    CHECK_NEAR(copy->balance(), 10001, 1e-9);

    // --- CSV round trip for every concrete type ---
    for (Account* a : {(Account*)&s2, (Account*)&c, (Account*)&fd, (Account*)&st}) {
        auto back = Account::fromCsv(a->toCsv());
        CHECK(back != nullptr);
        CHECK(back && back->typeCode() == a->typeCode());
        CHECK(back && back->accNo() == a->accNo());
        CHECK(back && std::fabs(back->balance() - a->balance()) < 0.005);
        CHECK(back && back->toCsv() == a->toCsv());
    }
    CHECK(SavingsAccount("S2", "T", 10000, now, 4.0).toCsv().size() == SavingsAccount("S2", "T", 99, now, 4.0).toCsv().size());   // fixed-width balance

    // --- closing ---
    SavingsAccount z("Z", "Z", 2500, now);
    CHECK_NEAR(z.closeAndPayout(), 2500, 1e-9);
    CHECK(!z.active());
    CHECK_THROWS(z.deposit(10), BankException);

    // --- formatting ---
    CHECK(util::money(1234567.5) == "\xE2\x82\xB9" "12,34,567.50");
    CHECK(util::money(-950) == "-\xE2\x82\xB9" "950.00");
    CHECK(util::maskAccount("SB1001") == "XXXX-1001");
    return finish("test_account");
}
