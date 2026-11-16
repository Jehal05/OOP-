#include "TestHarness.h"
#include "User.h"
#include "Utils.h"

int main() {
    Customer c("SB1", "Test User", "1234", "SB1");
    CHECK(c.verifyPin("1234"));
    CHECK(!c.verifyPin("1235"));
    CHECK(c.attemptsLeft() == 3);

    c.recordFailure(); c.recordFailure();
    CHECK(!c.locked() && c.attemptsLeft() == 1);
    c.recordFailure();                                  // 3rd failure locks
    CHECK(c.locked());
    c.unlock();
    CHECK(!c.locked() && c.failedAttempts() == 0);

    // PIN is stored as a salted hash, never in plain text
    std::string csv = c.toCsv();
    CHECK(csv.find("1234") == std::string::npos);
    Customer d("SB2", "Other", "1234", "SB2");
    CHECK(util::hashPin("1234", "salt1") != util::hashPin("1234", "salt2"));
    CHECK(c.toCsv().substr(c.toCsv().find("Test User")) != d.toCsv().substr(d.toCsv().find("Other")));

    // CSV round trip preserves hash, role and lock state
    c.recordFailure(); c.recordFailure(); c.recordFailure();
    auto back = User::fromCsv(c.toCsv());
    CHECK(back && back->locked() && back->verifyPin("1234") && back->role() == "CUSTOMER");
    CHECK(back && back->linkedAccount() == "SB1");

    Admin a("admin", "Boss", "secret99");
    auto ab = User::fromCsv(a.toCsv());
    CHECK(ab && ab->role() == "ADMIN" && ab->verifyPin("secret99"));

    a.setPin("newpass1");
    CHECK(!a.verifyPin("secret99") && a.verifyPin("newpass1"));
    return finish("test_security");
}
