# Nova Bank - Banking System in C++

A simple console banking program written in C++17 for the Object-Oriented Programming lab (mini project). It uses classes, inheritance, virtual functions, exceptions, the STL and file handling, with no libraries beyond the standard one.

## What it can do

- Four account types: Savings, Student, Current and Fixed Deposit
- Login with account number and PIN (3 wrong PINs lock the account, only the admin can unlock it)
- Deposit, withdraw, transfer, statement (filters and export to a text file), change PIN
- Loans: Home, Education and Personal, with EMI calculation, admin approval and EMI payments
- Simple fraud check: large amount, many debits in a short time, debit at an odd hour
- Admin menu: open accounts, view transactions, approve loans, review alerts, unlock accounts, pay interest, read the audit log
- All data is saved in plain CSV files in a `data` folder (created on first run with demo data)

## Files

| File | Purpose |
|---|---|
| `main.cpp` | Menus and user input |
| `Bank.h / Bank.cpp` | The bank: accounts, transactions, loans, alerts, saving and loading |
| `Account.h / Account.cpp` | `Account` base class and the four account types |
| `Loan.h / Loan.cpp` | `Loan` base class, Home / Education / Personal loans, EMI |
| `Fraud.h / Fraud.cpp` | `FraudRule` base class and the three rules |
| `Transaction.h` | One transaction record |
| `Exceptions.h` | Our own exception classes |
| `Utils.h` | Small helpers (text, money, dates, PIN hash) |

## Build and run

**Windows PowerShell** (install the compiler once with `winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT`, then reopen PowerShell):

```powershell
cd BankingSystem
g++ -std=c++17 -O2 -static (Get-ChildItem *.cpp).Name -o bank.exe
.\bank.exe
```

**Linux / macOS:**

```bash
cd BankingSystem
g++ -std=c++17 -O2 *.cpp -o bank
./bank
```

To start again with fresh demo data, delete the `data` folder.

## Demo logins

| Account | PIN | Type | Owner |
|---|---|---|---|
| SB1001 | 1234 | Savings | Asha Verma |
| CA2001 | 2345 | Current | Ravi Kumar |
| FD3001 | 3456 | Fixed Deposit | Meera Nair |
| ST1001 | 4567 | Student | Arjun Rao |
| admin | admin123 | Admin | |

## OOP concepts used

| Concept | Where |
|---|---|
| Abstract class and virtual functions | `Account`, `Loan`, `FraudRule` |
| Inheritance | `SavingsAccount` -> `StudentAccount`; `HomeLoan`, `EducationLoan`, `PersonalLoan` |
| Polymorphism | `account->withdrawalFee()`, `loan->emi()`, `rule->check(...)` |
| Encapsulation | Private/protected data, balance changes only through methods |
| Operator overloading | `operator<<` prints an `Account` |
| Exceptions | `BankException` and its subclasses (insufficient funds, locked, not found, invalid input) |
| Smart pointers and STL | `unique_ptr`, `vector`, `<algorithm>` |
| File handling | CSV load/save, append-only `audit.log`, statement export |

## Sample run

```
==== Customer menu - SB1001 ====
1. Account details
2. Deposit
...
Choose: 1
SB1001  Asha Verma  (Savings)  Rs 126800.00
Rules: 4.0% interest, minimum balance Rs 1000
Available to withdraw: Rs 125800.00

Choose: 2
Amount to deposit (Rs): 5000
Deposited. Reference TXN-20261117-000009. New balance Rs 131800.00
```

## Limitations

- The PIN hash is a simple educational one, not real security
- The PIN is visible while typing
- Admin login is fixed (`admin` / `admin123`)
- CSV files suit one user at a time, not many programs at once
