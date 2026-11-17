# Nova Bank - Banking & Management System

A complete **C++17 object-oriented banking application** for the terminal: customers and bank staff log in, move money, track spending, save towards goals, take loans, and get protected by a rule-based fraud detector, all as a plain text console program with file-based persistence.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue) ![tests](https://img.shields.io/badge/tests-218%20checks%20passing-brightgreen) ![valgrind](https://img.shields.io/badge/valgrind-0%20leaks%2C%200%20errors-brightgreen) ![deps](https://img.shields.io/badge/dependencies-none-lightgrey)

> Mini project for the Object-Oriented Programming lab (Experiment 11). It combines everything from Experiments 1-10: classes and access specifiers, constructors and the Rule of 5, inheritance, runtime and compile-time polymorphism, templates, STL, exceptions and file I/O.

## Contents
[Features](#features) | [Quick start](#quick-start) | [Demo logins](#demo-logins) | [Architecture](#architecture) | [Class diagrams](#class-diagrams) | [Flowcharts](#flowcharts) | [OOP concepts used](#oop-concepts-used-and-where) | [Data model](#data-model) | [Sample output](#sample-output) | [Testing](#testing-and-quality) | [Project structure](#project-structure) | [Limitations](#limitations-and-honest-notes) | [Viva Q&A](docs/VIVA.md)

---

## Features

**Core banking** - login with account number and PIN; deposit, withdraw, transfer; balance and account details; change PIN. Admins open, search, update and close accounts, view any account's history, apply interest and run reports. Four account types: **Savings** (minimum balance), **Student Savings** (zero minimum), **Current** (overdraft), **Fixed Deposit** (lock-in and early-exit fee).

| # | Feature | What it does |
|---|---|---|
| 1 | **Smart Statement** | Filter by date range, type, category and min/max amount; sort by date or amount; running-balance column; summary (credits, debits, net, average, largest); monthly grouping; export to a formatted `.txt` |
| 2 | **Spending Categories** | Eight categories (`enum class Category`), per-category totals and percentages, sorted from highest to lowest spending, month filter |
| 3 | **Savings Goals** | Name, target and deadline; contribute from the balance; progress percentage; required monthly saving; Active / Achieved / Overdue status |
| 4 | **Fraud Alerts** | Five polymorphic `FraudRule`s (large amount, rapid transactions, new payee high value, odd hour, repeated failures); alerts shown at next login and reviewed by admin |
| 5 | **Loan & EMI** | Home / Education / Personal loans (rate, tenure and limits differ per type), EMI formula, amortization table, admin approval, EMI payments, prepayment |
| 6 | **PIN Lockout & Audit Log** | 3 wrong PINs lock the user; only an admin unlocks; salted PIN hashes; append-only audit trail of logins, account changes, loan decisions, unlocks and fraud alerts |
| 7 | **Beneficiaries** | Save payees with a nickname, transfer by picking them, validation (exists, not yourself, no duplicates), new-payee monitoring for 24 h |
| 8 | **Transaction Receipts** | Reference IDs like `TXN-20261116-000123`, a receipt after every operation, saved to `receipts/`, lookup by reference |
| 9 | **Interest & FD Simulator** | Year-by-year projection comparing Savings, Student, FD and Current; maturity amounts; your own account's projection |

---

## Quick start (Windows PowerShell)

You need a C++17 compiler and `make`. No other libraries are used.

**1. Install the compiler once** (gives you `g++` and `mingw32-make`), then close and reopen PowerShell:
```powershell
winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
g++ --version
mingw32-make --version
```

**2. Build and run** from the project folder:
```powershell
cd C:\Users\Hp\OOP-\BankingSystem
mingw32-make
.\bank.exe
```
The first run creates a `data` folder with the demo accounts. If `mingw32-make` fails, compile directly:
```powershell
g++ -std=c++17 -O2 -Iinclude -static (Get-ChildItem src\*.cpp).FullName -o bank.exe
.\bank.exe
```
The Makefile links the C++ runtime statically on Windows; without it, Windows can pick up a different `libstdc++-6.dll` from your PATH and the program exits silently. The program prints plain ASCII only, so it looks the same in any PowerShell or Command Prompt window.

**3. Options**
```powershell
.\bank.exe --data mydata                # use another data folder
$env:BANK_TIMEOUT_SEC = 60; .\bank.exe   # auto-logout after 60 s idle
```
To reset to fresh demo data, delete the folder: `Remove-Item -Recurse data`.

**Linux / macOS / WSL**
```bash
cd BankingSystem
make && ./bank
```
Extra targets there: `make test`, `make valgrind`, `make check` (both), `make diagrams`, `make clean`.

## Demo logins

Demo data is created automatically the first time you run the program (it is regenerated if you delete `data/`).

| Role | Login | PIN / password | Account |
|---|---|---|---|
| Customer | `SB1001` | `1234` | Asha Verma - Savings, active loan, 2 goals, saved payee |
| Customer | `CA2001` | `2345` | Ravi Kumar - Current account with overdraft, one pending loan, one alert |
| Customer | `FD3001` | `3456` | Meera Nair - Fixed deposit (try an early withdrawal) |
| Customer | `ST1001` | `4567` | Arjun Rao - Student savings |
| Admin | `admin` | `admin123` | Approves loans, reviews alerts, unlocks users, reads the audit log |

Things worth trying: withdraw `60000` from `CA2001` to trigger a fraud alert; enter a wrong PIN three times to see the lockout and then unlock the user as admin; withdraw from `FD3001` to see the penalty; approve loan `L002` as admin.

---

## Architecture

A strict layering: the UI never contains business rules, and the business layer never prints.

<p align="center"><img src="docs/img/architecture.png" width="760"></p>

## Class diagrams

**Domain hierarchies** - three abstract bases, each with polymorphic behaviour:

<p align="center"><img src="docs/img/class_domain.png" width="900"></p>

**Services** - `Bank` owns seven `Repository<T>` instances, a `FraudDetector` (which owns five rules) and the `AuditLogger`:

<p align="center"><img src="docs/img/class_services.png" width="900"></p>

**Exception hierarchy:**

<p align="center"><img src="docs/img/exceptions.png" width="420"></p>

## Flowcharts

**Application flow:**

<p align="center"><img src="docs/img/app_flow.png" width="620"></p>

**Login and lockout:**

<p align="center"><img src="docs/img/login_flow.png" width="620"></p>

**Transfer (sequence diagram):**

<p align="center"><img src="docs/img/transfer_sequence.png" width="860"></p>

**Loan lifecycle:**

<p align="center"><img src="docs/img/loan_states.png" width="460"></p>

**Fraud screening:**

<p align="center"><img src="docs/img/fraud_flow.png" width="900"></p>

The Mermaid sources for every diagram are in [`docs/diagrams/`](docs/diagrams) (GitHub also renders `.mmd` files natively).

---

## OOP concepts used and where

| Concept | Where in the code |
|---|---|
| Classes, encapsulation, access specifiers | `Account` keeps balance private with a protected setter and public operations; `User` hides the hash and salt |
| Constructors (default, parameterized, copy, move) | `Account` family (copy via `clone()`, defaulted move), `Transaction`, `Loan`, `Repository` |
| Rule of 5 | `Repository<T>` (move-only because it owns `unique_ptr`s), `Account` (explicitly defaulted), virtual destructors on every base |
| Single inheritance | `Account -> CurrentAccount / FixedDepositAccount`, `User -> Customer / Admin`, `Loan -> HomeLoan / EducationLoan / PersonalLoan` |
| Multilevel inheritance | `Account -> SavingsAccount -> StudentSavingsAccount` |
| Runtime polymorphism | `Account::calculateInterest()/spendable()/withdrawalFee()`, `Loan::annualRate()/maxMonths()`, `FraudRule::check()` through `unique_ptr` to the base |
| Abstract classes | `Account`, `User`, `Loan`, `FraudRule` |
| Operator overloading | `Transaction::operator<` and `<<`, `Account::operator+=` and `==`, `SavingsGoal::operator+=`, `Account` stream output |
| Function overloading | `ui::` helpers, `Loan::create` factory variants, formatting helpers |
| Templates | `Repository<T>` works for `Account`, `User`, `Transaction`, `Loan`, `SavingsGoal`, `Beneficiary`, `Alert` |
| STL containers | `vector`, `map`, `unordered_map` (the O(1) key index inside `Repository<T>`), `optional` |
| STL algorithms | `sort`, `stable_sort`, `remove_if`, `any_of`, `accumulate`, `max_element`, `reverse` |
| Custom exceptions | `BankException` and 7 subclasses, caught at the menu level so bad input never crashes the app |
| File I/O | CSV via `fstream`, append-only transaction log and audit log, in-place record patching with `tellg()`/`seekp()`, `.txt` statement and receipt export |
| Smart pointers | `unique_ptr` for every owned account, user, loan, record and fraud rule; no raw owning pointers |
| Separation of concerns | All terminal output lives in `ConsoleUI`, `App` and `AdminApp` |

## Data model

<p align="center"><img src="docs/img/er_diagram.png" width="700"></p>

| File | Contents |
|---|---|
| `users.csv` | Logins: role, salt, PIN hash, failed attempts, locked flag, linked account |
| `accounts.csv` | Accounts; the balance is a **fixed-width** field so it can be patched in place |
| `transactions.csv` | Append-only ledger with reference IDs and balance after each entry |
| `beneficiaries.csv`, `goals.csv`, `loans.csv`, `alerts.csv` | Per-feature records |
| `audit.log` | Append-only `timestamp / actor / action / details` lines |

Free text (notes, alert messages) is escaped so commas and newlines survive the CSV round trip.

## Console design

The program is deliberately plain: ordinary text menus, no colors, no box drawing, no charts.

- **Same layout on every screen:** a header, a line showing where you are (`Home > Customer > Statement`), the content, then a numbered menu.
- **Messages:** `[SUCCESS]`, `[ERROR]`, `[WARNING]` and `[INFO]` lines.
- **Amounts** use Indian digit grouping, for example `Rs 12,34,567.50`.
- **Safe input:** every prompt checks what you typed and asks again; amounts accept at most 2 decimals; the PIN shows as `*`; risky actions (transfer, close account, prepay, FD penalty) ask for y/n confirmation.
- **Idle auto-logout** after 5 minutes (change it with `BANK_TIMEOUT_SEC`).

## Sample output

A short excerpt (login, dashboard and spending summary) from a real run:

```text
==============================================================================
 NOVA BANK - Banking Management System
==============================================================================
 Home > Customer login

 > Account number SB1001
 > PIN ****

==============================================================================
 NOVA BANK - Banking Management System
==============================================================================
 Home > Customer > Dashboard

 Good morning, Asha Verma

 Account
 -------
   Asha Verma   XXXX-1001  |  Savings
   Available balance   Rs 1,59,735.13
   4.00% p.a. simple interest | minimum balance Rs 1,000.00


 Recent activity
 ---------------
 Date          Type                 Amount         Balance
 ------------  ------------  -------------  --------------
 16 Nov 09:45  Withdraw         -Rs 780.00  Rs 1,59,735.13
 15 Nov 17:15  Withdraw       -Rs 3,200.00  Rs 1,60,515.13
 14 Nov 11:30  Transfer Out  -Rs 15,000.00  Rs 1,63,715.13
 1 active loan(s). Next EMI is shown under [5] Loans & EMI.

 Menu
 ----
   [1] Transactions              deposit, withdraw, transfer, balance
   [2] Smart statement           filters, sorting, export, receipts
   [3] Spending insights         category breakdown
   [4] Savings goals             targets and progress
   [5] Loans & EMI               apply, pay EMI, amortization
   [6] Beneficiaries             saved payees
   [7] Interest & FD simulator   what-if projections
   [8] Security & alerts         fraud alerts, change PIN
   [9] Logout                    

 > Choose an option 1
  ...

 Spending by category  (all time)
 --------------------------------
  Rent       Rs 65,000.00  54.5%
  Shopping   Rs 14,200.00  11.9%
  Food       Rs 11,780.00   9.9%
  Travel     Rs 9,900.00   8.3%
  Education  Rs 8,000.00   6.7%
  Bills      Rs 6,750.00   5.7%
  Health     Rs 3,700.00   3.1%

 Total spent: Rs 1,19,330.00   Top category: Rent

```

Full sessions are in [`docs/sample_customer_session.txt`](docs/sample_customer_session.txt), [`docs/sample_admin_session.txt`](docs/sample_admin_session.txt) and [`docs/sample_lockout_fraud_session.txt`](docs/sample_lockout_fraud_session.txt). They are recorded from the scripted scenarios in [`scenarios/`](scenarios).

## Testing and quality

| Check | Command | Result |
|---|---|---|
| Unit and integration tests | `make test` | 7 suites, **218 checks** pass (accounts, loans, fraud rules, security, repository, statements, full `Bank` workflows) |
| Memory safety | `make valgrind` | **0 leaks, 0 errors** across every scenario and the integration tests |
| Compiler warnings | `-Wall -Wextra` | none |

The scripted sessions in [`scenarios/`](scenarios) drive the real program end to end (they also run under Valgrind).

## Project structure

```text
BankingSystem/
├── include/        headers (Account, User, Loan, Fraud, Bank, Repository<T>, ConsoleUI, App ...)
├── src/            implementations + main.cpp
├── tests/          7 test suites + a tiny assert harness
├── scenarios/      scripted demo sessions (customer, admin, lockout and fraud)
├── tools/          scenario reader and Valgrind script
├── docs/           diagrams (.mmd sources and images), sample sessions, VIVA.md
├── data/ statements/ receipts/    created at runtime (git-ignored)
└── Makefile
```

About 4,000 lines of C++ in `include/`, `src/` and `tests/`.

## Limitations and honest notes

- **PIN hashing** is an iterated, salted FNV-1a: fine to demonstrate the idea, not production-grade (use bcrypt or argon2 for real systems).
- **Storage** is plain CSV with a single process; there is no locking, so do not run two instances on the same `data/` folder.
- **Idle auto-logout** is checked when the next input arrives (a console program cannot interrupt a blocking read).
- **Demo data** is generated relative to today's date and is only created when `data/` is empty.
- **Windows** builds use MinGW g++ and a static link (see Quick start).

## Repository branches
| Branch | Contents |
|---|---|
| `main` | Index of this repository |
| `experiment-01` | Experiment 1: Data types and I/O |
| `experiment-02` | Experiment 2: Class hierarchy |
| `experiment-03` | Experiment 3: Constructors and Rule of 5 |
| `experiment-04` | Experiment 4: Inheritance |
| `experiment-05` | Experiment 5: Runtime polymorphism |
| `experiment-06` | Experiment 6: Overloading |
| `experiment-07` | Experiment 7: Templates |
| `experiment-08` | Experiment 8: STL |
| `experiment-09` | Experiment 9: Exceptions |
| `experiment-10` | Experiment 10: File handling |
| `mini-project` | Nova Bank, a C++17 banking system for the terminal |

See the `main` branch README for an overview of all ten experiments.

## Author

Built by **Jehal05** as the OOP lab mini project. Experiments 1-10 live on the `experiment-01` ... `experiment-10` branches of this repository.
