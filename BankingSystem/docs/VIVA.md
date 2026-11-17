# Viva cheat sheet - Nova Bank (Banking & Management System)

**Q1. What does the project do?**
A console banking system with a customer side and an admin side. Data is saved in CSV files, and the output is plain text so it runs the same in any terminal.

**Q2. Where did you use inheritance?**
`Account -> SavingsAccount -> StudentSavingsAccount` is multilevel inheritance; `Account -> CurrentAccount / FixedDepositAccount` is single inheritance. The same idea is used in `User -> Customer / Admin` and `Loan -> HomeLoan / EducationLoan / PersonalLoan`.

**Q3. Where is runtime polymorphism used?**
`Account::calculateInterest()` (simple, compound or none), `Account::spendable()` and `withdrawalFee()` (overdraft, FD penalty), `Loan::annualRate()` and `maxMonths()`, and `FraudRule::check()`. `Bank` holds `unique_ptr<Account>` and `unique_ptr<FraudRule>` and calls them through the base class.

**Q4. Where is compile-time polymorphism used?**
Operator overloading: `Transaction::operator<` (sorting) and `operator<<`, `Account::operator+=` and `operator==`, `SavingsGoal::operator+=`. Templates: `Repository<T>`.

**Q5. What is an abstract class here, and why?**
`Account`, `User`, `Loan` and `FraudRule` each have pure virtual functions, so they cannot be instantiated. They define the contract that every concrete type must follow.

**Q6. Explain the Rule of 5 in this project.**
`Repository<T>` owns `unique_ptr`s, so copying is deleted and moving is defaulted. `Account` owns no raw resources, so its five special members are explicitly defaulted. A virtual destructor lets `delete` through a base pointer work.

**Q7. How is memory managed?**
There are no raw owning pointers. Ownership uses `unique_ptr`/`make_unique`, and `tools/valgrind_all.sh` shows zero leaks and zero errors across the scenarios and tests.

**Q8. How do you store data?**
`Repository<T>` reads and writes CSV with `fstream`. New transactions are appended (`ios::app`). Account balances are fixed-width so a record can be patched in place with `tellg()` to find it and `seekp()` to overwrite it.

**Q9. Why are balances zero-padded in `accounts.csv`?**
So a balance change keeps the line length identical, which makes in-place `seekp` patching possible. If the length changes (for example a renamed holder), the repository falls back to rewriting the file.

**Q10. How does the PIN lockout work?**
Each wrong PIN increments `failed_`. At 3 the user is locked, `AccountLockedException` is thrown, and only an admin can unlock. Each step is written to the audit log.

**Q11. Are PINs stored safely?**
They are stored as salted, iterated hashes, never in plain text. The hash is an iterated FNV-1a, which is fine for a teaching project but not production grade. A real system would use bcrypt or argon2.

**Q12. How is the audit log protected from edits?**
The application only opens it with `ios::app` and has no code path that rewrites or deletes entries.

**Q13. How does the fraud system work?**
After each deposit, withdrawal and outgoing transfer, `FraudDetector` builds a `FraudContext` and runs five `FraudRule` objects. Each rule that fires creates an `Alert`, which the customer sees on their dashboard and the admin can review.

**Q14. How is EMI computed?**
`EMI = P*r*(1+r)^n / ((1+r)^n - 1)` with `r = annual rate / 12 / 100`. The amortization table splits each EMI into interest (`balance * r`) and principal, and the last installment absorbs rounding drift so the final balance is exactly zero.

**Q15. How do exceptions flow?**
Domain errors derive from `BankException`. `Bank` throws them, and `App` catches `BankException` at the menu level to print an [ERROR] message, so bad input never crashes the program.

**Q16. What STL do you use?**
`vector` for records, `unordered_map` as the O(1) key index inside `Repository<T>` (this is how beneficiaries, accounts and transactions are found by key), `map` for ordered reports (monthly grouping, daily totals), and the algorithms `sort`, `stable_sort`, `remove_if`, `any_of`, `accumulate`, `max_element`.

**Q17. How do you separate UI from logic?**
All printing and input lives in `ConsoleUI`, `App` and `AdminApp`. `Bank`, `Account`, `Loan` and the rest never write to the console, which is why they are unit-testable.

**Q18. How is the console output kept simple and readable?**
Every screen has the same layout (header, breadcrumb, content, numbered menu), messages are tagged `[SUCCESS]`, `[ERROR]`, `[WARNING]` or `[INFO]`, and there are no colors or special characters, so it looks the same in any terminal.

**Q19. How did you test it?**
Seven test suites with 218 checks (`make test`), scripted end-to-end scenarios and Valgrind (`make check`).

**Q20. What would you improve next?**
A real database, stronger password hashing, concurrent sessions with locking, and a GUI or web front end.
