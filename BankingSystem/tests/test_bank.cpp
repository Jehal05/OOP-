// Integration tests against the real Bank controller using the seeded demo data.
#include "TestHarness.h"
#include "Bank.h"
#include "Utils.h"

int main() {
    std::string dir = tempDir("bank");
    Bank bank(dir);
    bank.load();                                                   // seeds demo data
    CHECK(bank.allAccounts().size() == 4);

    // ---- authentication and lockout ----
    CHECK_THROWS(bank.login("SB1001", "0000", "CUSTOMER"), AuthenticationException);
    CHECK_THROWS(bank.login("SB1001", "0000", "CUSTOMER"), AuthenticationException);
    CHECK_THROWS(bank.login("SB1001", "0000", "CUSTOMER"), AccountLockedException);        // 3rd failure locks
    CHECK_THROWS(bank.login("SB1001", "1234", "CUSTOMER"), AccountLockedException);        // even with the right PIN
    CHECK_THROWS(bank.login("SB1001", "1234", "ADMIN"), AuthenticationException);          // wrong role
    bank.unlockUser("admin", "SB1001");
    CHECK(bank.login("SB1001", "1234", "CUSTOMER").id() == "SB1001");
    CHECK(bank.login("admin", "admin123", "ADMIN").role() == "ADMIN");
    CHECK_THROWS(bank.login("nobody", "1234", "CUSTOMER"), AuthenticationException);
    auto audit = bank.audit().readAll();
    bool sawLock = false;
    for (const auto& l : audit) if (l.find("ACCOUNT_LOCKED") != std::string::npos) sawLock = true;
    CHECK(sawLock);

    // ---- deposit / withdraw / balance consistency ----
    double b0 = bank.account("SB1001").balance();
    const Transaction& d = bank.deposit("SB1001", 1000);
    CHECK_NEAR(d.balanceAfter(), b0 + 1000, 1e-9);
    bank.withdraw("SB1001", 400, Category::Food);
    CHECK_NEAR(bank.account("SB1001").balance(), b0 + 600, 1e-9);
    CHECK_THROWS(bank.withdraw("SB1001", 99999999, Category::Food), InsufficientFundsException);
    CHECK_THROWS(bank.deposit("SB1001", -5), InvalidAmountException);
    CHECK_THROWS(bank.deposit("NOPE", 5), AccountNotFoundException);

    // ---- transfer moves money atomically and creates both legs ----
    double a0 = bank.account("SB1001").balance(), r0 = bank.account("CA2001").balance();
    bank.transfer("SB1001", "CA2001", 2000, Category::Rent);
    CHECK_NEAR(bank.account("SB1001").balance(), a0 - 2000, 1e-9);
    CHECK_NEAR(bank.account("CA2001").balance(), r0 + 2000, 1e-9);
    CHECK_THROWS(bank.transfer("SB1001", "SB1001", 10, Category::Other), InvalidAmountException);
    CHECK_THROWS(bank.transfer("SB1001", "ZZ", 10, Category::Other), AccountNotFoundException);

    // ---- persistence: reload from disk gives identical balances ----
    double saved = bank.account("SB1001").balance();
    Bank again(dir);
    again.load();
    CHECK_NEAR(again.account("SB1001").balance(), saved, 0.005);
    CHECK(again.history("SB1001").size() == bank.history("SB1001").size());

    // ---- fixed deposit early-withdrawal penalty creates a Fee transaction ----
    size_t before = bank.history("FD3001").size();
    bank.withdraw("FD3001", 10000, Category::Other);
    CHECK_NEAR(bank.account("FD3001").balance(), 200000 - 10000 - 100, 1e-6);
    CHECK(bank.history("FD3001").size() == before + 2);

    // ---- fraud: a large withdrawal raises an alert and an audit entry ----
    size_t alertsBefore = bank.alertsOf("CA2001").size();
    bank.withdraw("CA2001", 60000, Category::Shopping);
    CHECK(bank.alertsOf("CA2001").size() > alertsBefore);
    CHECK(!bank.unseenAlerts("CA2001").empty());

    // ---- beneficiaries ----
    CHECK(bank.payeesOf("SB1001").size() == 1);
    bank.addPayee("SB1001", "Meera", "FD3001");
    CHECK(bank.findPayee("SB1001", "meera") != nullptr);
    CHECK_THROWS(bank.addPayee("SB1001", "Dup", "FD3001"), BankException);        // same account twice
    CHECK_THROWS(bank.addPayee("SB1001", "Me", "SB1001"), BankException);         // self
    CHECK_THROWS(bank.addPayee("SB1001", "Ghost", "XX0000"), AccountNotFoundException);
    bank.removePayee("SB1001", "Meera");
    CHECK(bank.findPayee("SB1001", "Meera") == nullptr);

    // ---- goals ----
    auto goals = bank.goalsOf("SB1001");
    CHECK(goals.size() == 2);
    double saved0 = goals[0]->saved();
    bank.contributeToGoal(goals[0]->id(), 1000);
    CHECK_NEAR(bank.goalsOf("SB1001")[0]->saved(), saved0 + 1000, 1e-9);
    CHECK_THROWS(bank.contributeToGoal(goals[0]->id(), 99999999), InvalidAmountException);
    const SavingsGoal& g = bank.createGoal("SB1001", "Bike", 50000, util::addDays(util::now(), 90));
    CHECK(g.status(util::now()) == "Active");
    CHECK_THROWS(bank.createGoal("SB1001", "Past", 100, util::addDays(util::now(), -1)), BankException);

    // ---- loans: apply -> approve -> EMI -> prepay ----
    CHECK_THROWS(bank.applyLoan("SB1001", "PER", 9000000, 12), LoanException);    // above limit
    CHECK_THROWS(bank.applyLoan("SB1001", "PER", 50000, 100), LoanException);     // tenure too long
    CHECK_THROWS(bank.applyLoan("SB1001", "XXX", 50000, 12), LoanException);
    CHECK(bank.pendingLoans().size() == 1);                                       // seeded L002
    double ravi = bank.account("CA2001").balance();
    bank.approveLoan("admin", "L002");
    CHECK_NEAR(bank.account("CA2001").balance(), ravi + 150000, 1e-9);
    CHECK_THROWS(bank.approveLoan("admin", "L002"), LoanException);               // already active
    bank.payEMI("L002");
    CHECK(bank.findLoan("L002")->paid() == 1);
    double out = bank.findLoan("L002")->outstanding();
    double bal = bank.account("CA2001").balance();
    bank.prepayLoan("L002");
    CHECK(bank.findLoan("L002")->status() == LoanStatus::Closed);
    CHECK_NEAR(bank.account("CA2001").balance(), bal - out, 0.01);
    const Loan& nl = bank.applyLoan("ST1001", "EDU", 100000, 24);
    bank.rejectLoan("admin", nl.id(), "Insufficient income proof");
    CHECK(bank.findLoan(nl.id())->status() == LoanStatus::Rejected);

    // ---- interest runs once per month ----
    auto first = bank.applyMonthlyInterest("admin");
    bool credited = false;
    for (const auto& r : first) if (r.status == "Credited") credited = true;
    CHECK(credited);
    for (const auto& r : bank.applyMonthlyInterest("admin")) CHECK(r.status != "Credited");

    // ---- admin account lifecycle ----
    std::string no = bank.openAccount("admin", "SAV", "New Customer", 5000, "4321", 4.0, 1000);
    CHECK(no == "SB1002");
    CHECK(bank.login(no, "4321", "CUSTOMER").id() == no);
    bank.updateHolder("admin", no, "Renamed Customer");
    CHECK(bank.account(no).holder() == "Renamed Customer");
    CHECK_THROWS(bank.openAccount("admin", "SAV", "Xy", 10, "4321", 4, 1000), InvalidAmountException);   // below minimum
    CHECK_THROWS(bank.openAccount("admin", "SAV", "Xy", 5000, "12", 4, 1000), BankException);           // bad PIN
    CHECK_NEAR(bank.closeAccount("admin", no), 5000, 1e-9);
    CHECK_THROWS(bank.deposit(no, 10), BankException);
    CHECK_THROWS(bank.login(no, "4321", "CUSTOMER"), BankException);
    CHECK_THROWS(bank.closeAccount("admin", "SB1001"), BankException);            // still has an active loan

    // ---- PIN change ----
    User& u = bank.login("ST1001", "4567", "CUSTOMER");
    CHECK_THROWS(bank.changePin(u, "0000", "1111"), AuthenticationException);
    CHECK_THROWS(bank.changePin(u, "4567", "12"), BankException);
    CHECK_THROWS(bank.changePin(u, "4567", "4567"), BankException);
    bank.changePin(u, "4567", "9999");
    CHECK(bank.login("ST1001", "9999", "CUSTOMER").id() == "ST1001");

    // ---- reports ----
    CHECK(bank.richest(2).size() == 2 && bank.richest(2)[0]->balance() >= bank.richest(2)[1]->balance());
    CHECK(bank.totalBalance() > 0 && bank.totalDeposits() > 0);
    CHECK(!bank.periodTotals(true).empty());
    CHECK(bank.dormant(1000).empty());

    // ---- simulator ----
    SavingsAccount probe("-", "-", 1000, 0, 5.0);
    auto rows = Bank::simulate(probe, 1000, 3);
    CHECK_NEAR(rows[2].simple, 1150.0, 0.01);
    CHECK_NEAR(rows[2].compound, 1157.63, 0.01);

    // ---- unique, formatted reference IDs ----
    const Transaction& t = bank.deposit("ST1001", 1);
    CHECK(t.refId().rfind("TXN-" + util::compactDate(util::now()) + "-", 0) == 0);
    CHECK(bank.findTransaction(t.refId()) != nullptr);

    std::filesystem::remove_all(dir);
    return finish("test_bank");
}
