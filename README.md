# Nova Bank - Banking & Management System

A complete **C++17 object-oriented banking application** for the terminal: customers and bank staff log in, move money, track spending, save towards goals, take loans, and get protected by a rule-based fraud detector, all with a bank-grade console UI and file-based persistence.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue) ![tests](https://img.shields.io/badge/tests-218%20checks%20passing-brightgreen) ![valgrind](https://img.shields.io/badge/valgrind-0%20leaks%2C%200%20errors-brightgreen) ![deps](https://img.shields.io/badge/dependencies-none-lightgrey)

> Mini project for the Object-Oriented Programming lab (Experiment 11). It combines everything from Experiments 1-10: classes and access specifiers, constructors and the Rule of 5, inheritance, runtime and compile-time polymorphism, templates, STL, exceptions and file I/O.

## Contents
[Screenshots](#screenshots) | [Features](#features) | [Quick start](#quick-start) | [Demo logins](#demo-logins) | [Architecture](#architecture) | [Class diagrams](#class-diagrams) | [Flowcharts](#flowcharts) | [OOP concepts used](#oop-concepts-used-and-where) | [Data model](#data-model) | [UI/UX design](#uiux-design) | [Testing](#testing-and-quality) | [Project structure](#project-structure) | [Limitations](#limitations-and-honest-notes) | [Viva Q&A](BankingSystem/docs/VIVA.md)

---

## Screenshots

Every image below is rendered from the **real program output** (`make screenshots` replays the scripted sessions in `scenarios/` and draws the terminal frames), so they cannot drift from the code.

### Customer

| | |
|---|---|
| **Dashboard** - balance card, recent activity, unread alerts, grouped menu<br><img src="BankingSystem/docs/img/screens/02_dashboard.png" width="480"> | **Deposit with receipt** - unique reference ID, receipt saved to `receipts/`<br><img src="BankingSystem/docs/img/screens/03_deposit_receipt.png" width="480"> |
| **Transfer** - saved payees, confirmation card, Y/N guard<br><img src="BankingSystem/docs/img/screens/04_transfer.png" width="480"> | **Smart statement** - filtered by type, sorted by amount, running balance, summary<br><img src="BankingSystem/docs/img/screens/05_smart_statement.png" width="480"> |
| **Monthly summary**<br><img src="BankingSystem/docs/img/screens/06_monthly_summary.png" width="480"> | **Statement export** to a bank-style `.txt` file<br><img src="BankingSystem/docs/img/screens/07_statement_export.png" width="480"> |
| **Spending categories** - text bar chart, top category highlighted<br><img src="BankingSystem/docs/img/screens/08_spending_chart.png" width="480"> | **Savings goals** - progress bars and required monthly saving<br><img src="BankingSystem/docs/img/screens/09_savings_goals.png" width="480"> |
| **EMI calculator** - estimate without applying<br><img src="BankingSystem/docs/img/screens/10_emi_calculator.png" width="480"> | **Amortization table** - month-by-month interest and principal<br><img src="BankingSystem/docs/img/screens/11_amortization.png" width="480"> |
| **Interest & FD simulator** - same amount in each account type<br><img src="BankingSystem/docs/img/screens/12_simulator.png" width="480"> | **Home screen**<br><img src="BankingSystem/docs/img/screens/01_home.png" width="480"> |

### Security and fraud

| | |
|---|---|
| **PIN lockout** after 3 wrong attempts (masked PIN entry)<br><img src="BankingSystem/docs/img/screens/14_lockout.png" width="480"> | **Fraud alerts** raised by the polymorphic rules<br><img src="BankingSystem/docs/img/screens/13_fraud_alerts.png" width="480"> |
| **FD early-withdrawal penalty** shown before the money moves<br><img src="BankingSystem/docs/img/screens/15_fd_penalty.png" width="480"> | |

### Admin

| | |
|---|---|
| **Admin dashboard** - pending loans, alerts, locked users<br><img src="BankingSystem/docs/img/screens/16_admin_dashboard.png" width="480"> | **Open account** - creates the account and the customer login<br><img src="BankingSystem/docs/img/screens/17_admin_open.png" width="480"> |
| **Loan approvals**<br><img src="BankingSystem/docs/img/screens/18_admin_loans.png" width="480"> | **Fraud alert review**<br><img src="BankingSystem/docs/img/screens/19_admin_fraud.png" width="480"> |
| **Reports** - richest accounts chart<br><img src="BankingSystem/docs/img/screens/20_admin_richest.png" width="480"> | **Append-only audit log**<br><img src="BankingSystem/docs/img/screens/21_admin_audit.png" width="480"> |

---

## Features

**Core banking** - login with account number and PIN; deposit, withdraw, transfer; balance and account details; change PIN. Admins open, search, update and close accounts, view any account's history, apply interest and run reports. Four account types: **Savings** (minimum balance), **Student Savings** (zero minimum), **Current** (overdraft), **Fixed Deposit** (lock-in and early-exit fee).

| # | Feature | What it does |
|---|---|---|
| 1 | **Smart Statement** | Filter by date range, type, category and min/max amount; sort by date or amount; running-balance column; summary (credits, debits, net, average, largest); monthly grouping; export to a formatted `.txt` |
| 2 | **Spending Categories** | Eight categories (`enum class Category`), per-category totals and percentages, text bar chart with the top category highlighted, month filter |
| 3 | **Savings Goals** | Name, target and deadline; contribute from the balance; progress bar; required monthly saving; Active / Achieved / Overdue status |
| 4 | **Fraud Alerts** | Five polymorphic `FraudRule`s (large amount, rapid transactions, new payee high value, odd hour, repeated failures); alerts shown at next login and reviewed by admin |
| 5 | **Loan & EMI** | Home / Education / Personal loans (rate, tenure and limits differ per type), EMI formula, amortization table, admin approval, EMI payments, prepayment |
| 6 | **PIN Lockout & Audit Log** | 3 wrong PINs lock the user; only an admin unlocks; salted PIN hashes; append-only audit trail of logins, account changes, loan decisions, unlocks and fraud alerts |
| 7 | **Beneficiaries** | Save payees with a nickname, transfer by picking them, validation (exists, not yourself, no duplicates), new-payee monitoring for 24 h |
| 8 | **Transaction Receipts** | Reference IDs like `TXN-20261116-000123`, a receipt card after every operation, saved to `receipts/`, lookup by reference |
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
The Makefile links the C++ runtime statically on Windows; without it, Windows can pick up a different `libstdc++-6.dll` from your PATH and the program exits silently.

**3. Options**
```powershell
.\bank.exe --ascii          # plain boxes if the symbols look garbled
.\bank.exe --no-color       # turn off colors
.\bank.exe --data mydata    # use another data folder
$env:BANK_TIMEOUT_SEC = 60; .\bank.exe     # auto-logout after 60 s idle
chcp 65001                  # run first if the box characters or the rupee sign look wrong
```
To reset to fresh demo data, delete the folder: `Remove-Item -Recurse data`.

**Linux / macOS / WSL**
```bash
cd BankingSystem
make && ./bank
```
Extra targets there: `make test`, `make widths`, `make valgrind`, `make check` (all three), `make screenshots`, `make diagrams`, `make clean`.

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

<p align="center"><img src="BankingSystem/docs/img/architecture.png" width="760"></p>

## Class diagrams

**Domain hierarchies** - three abstract bases, each with polymorphic behaviour:

<p align="center"><img src="BankingSystem/docs/img/class_domain.png" width="900"></p>

**Services** - `Bank` owns seven `Repository<T>` instances, a `FraudDetector` (which owns five rules) and the `AuditLogger`:

<p align="center"><img src="BankingSystem/docs/img/class_services.png" width="900"></p>

**Exception hierarchy:**

<p align="center"><img src="BankingSystem/docs/img/exceptions.png" width="420"></p>

## Flowcharts

**Application flow:**

<p align="center"><img src="BankingSystem/docs/img/app_flow.png" width="620"></p>

**Login and lockout:**

<p align="center"><img src="BankingSystem/docs/img/login_flow.png" width="620"></p>

**Transfer (sequence diagram):**

<p align="center"><img src="BankingSystem/docs/img/transfer_sequence.png" width="860"></p>

**Loan lifecycle:**

<p align="center"><img src="BankingSystem/docs/img/loan_states.png" width="460"></p>

**Fraud screening:**

<p align="center"><img src="BankingSystem/docs/img/fraud_flow.png" width="900"></p>

The Mermaid sources for every diagram are in [`docs/diagrams/`](BankingSystem/docs/diagrams) (GitHub also renders `.mmd` files natively).

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

<p align="center"><img src="BankingSystem/docs/img/er_diagram.png" width="700"></p>

| File | Contents |
|---|---|
| `users.csv` | Logins: role, salt, PIN hash, failed attempts, locked flag, linked account |
| `accounts.csv` | Accounts; the balance is a **fixed-width** field so it can be patched in place |
| `transactions.csv` | Append-only ledger with reference IDs and balance after each entry |
| `beneficiaries.csv`, `goals.csv`, `loans.csv`, `alerts.csv` | Per-feature records |
| `audit.log` | Append-only `timestamp / actor / action / details` lines |

Free text (notes, alert messages) is escaped so commas and newlines survive the CSV round trip.

## UI/UX design

- **Consistent screens:** header banner, breadcrumb (`Home > Customer > Statement`), body, and a footer hint.
- **Meaningful color, never color alone:** green for credits and success, red for debits and errors, yellow for warnings, cyan for headings, grey for hints. Every color is paired with a symbol or word (`+`/`-`, `SUCCESS`, `ERROR`), and `--no-color` / `--ascii` are supported.
- **Indian digit grouping** (`₹12,34,567.50`) and right-aligned amounts.
- **Safe input:** every prompt validates and re-asks; amounts accept at most 2 decimals; PIN entry is masked on a real terminal; destructive actions (transfer, close account, prepay, FD penalty) ask for Y/N confirmation.
- **Feedback:** success/error banners with word-wrapping, receipt cards, attempts-remaining on a wrong PIN, and an idle auto-logout.
- **80-column rule:** `make widths` fails if any screen line is wider than 80 columns.

## Testing and quality

| Check | Command | Result |
|---|---|---|
| Unit and integration tests | `make test` | 7 suites, **218 checks** pass (accounts, loans, fraud rules, security, repository, statements, full `Bank` workflows) |
| Layout | `make widths` | no screen line over 80 columns across all scenarios |
| Memory safety | `make valgrind` | **0 leaks, 0 errors** across every scenario and the integration tests |
| Compiler warnings | `-Wall -Wextra` | none |

The scripted sessions in [`scenarios/`](BankingSystem/scenarios) drive the real binary end to end and are also the source of the screenshots.

## Project structure

```text
BankingSystem/
├── include/        headers (Account, User, Loan, Fraud, Bank, Repository<T>, ConsoleUI, App ...)
├── src/            implementations + main.cpp
├── tests/          7 test suites + a tiny assert harness
├── scenarios/      scripted demo sessions (customer, admin, lockout and fraud)
├── tools/          screenshot renderer, scenario runner, width checker, Valgrind script
├── docs/           diagrams (.mmd sources), rendered images, VIVA.md
├── data/ statements/ receipts/    created at runtime (git-ignored)
└── Makefile
```

About 4,200 lines of C++ in `include/`, `src/` and `tests/`.

## Limitations and honest notes

- **PIN hashing** is an iterated, salted FNV-1a: fine to demonstrate the idea, not production-grade (use bcrypt or argon2 for real systems).
- **Storage** is plain CSV with a single process; there is no locking, so do not run two instances on the same `data/` folder.
- **Idle auto-logout** is checked when the next input arrives (a console program cannot interrupt a blocking read).
- **Demo data** is generated relative to today's date and is only created when `data/` is empty.
- **Windows** builds use MinGW g++ and a static link (see Quick start); the terminal needs UTF-8 for the box characters, otherwise use `--ascii`.

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
