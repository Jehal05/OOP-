# Nova Bank - Banking & Management System

A complete **C++17 object-oriented banking application** for the terminal: customers and bank staff log in, move money, track spending, save towards goals, take loans, and get protected by a rule-based fraud detector, all with a bank-grade console UI and file-based persistence.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue) ![tests](https://img.shields.io/badge/tests-218%20checks%20passing-brightgreen) ![valgrind](https://img.shields.io/badge/valgrind-0%20leaks%2C%200%20errors-brightgreen) ![deps](https://img.shields.io/badge/dependencies-none-lightgrey)

> Mini project for the Object-Oriented Programming lab (Experiment 11). It combines everything from Experiments 1-10: classes and access specifiers, constructors and the Rule of 5, inheritance, runtime and compile-time polymorphism, templates, STL, exceptions and file I/O.

## Contents
[Screenshots](#screenshots) | [Features](#features) | [Quick start](#quick-start) | [Demo logins](#demo-logins) | [Architecture](#architecture) | [Web version](#web-version-runs-on-localhost) | [Class diagrams](#class-diagrams) | [Flowcharts](#flowcharts) | [OOP concepts used](#oop-concepts-used-and-where) | [Data model](#data-model) | [UI/UX design](#uiux-design) | [Testing](#testing-and-quality) | [Project structure](#project-structure) | [Limitations](#limitations-and-honest-notes) | [Viva Q&A](docs/VIVA.md)

---

## Screenshots

Every image below is rendered from the **real program output** (`make screenshots` replays the scripted sessions in `scenarios/` and draws the terminal frames), so they cannot drift from the code.

### Customer

| | |
|---|---|
| **Dashboard** - balance card, recent activity, unread alerts, grouped menu<br><img src="docs/img/screens/02_dashboard.png" width="480"> | **Deposit with receipt** - unique reference ID, receipt saved to `receipts/`<br><img src="docs/img/screens/03_deposit_receipt.png" width="480"> |
| **Transfer** - saved payees, confirmation card, Y/N guard<br><img src="docs/img/screens/04_transfer.png" width="480"> | **Smart statement** - filtered by type, sorted by amount, running balance, summary<br><img src="docs/img/screens/05_smart_statement.png" width="480"> |
| **Monthly summary**<br><img src="docs/img/screens/06_monthly_summary.png" width="480"> | **Statement export** to a bank-style `.txt` file<br><img src="docs/img/screens/07_statement_export.png" width="480"> |
| **Spending categories** - text bar chart, top category highlighted<br><img src="docs/img/screens/08_spending_chart.png" width="480"> | **Savings goals** - progress bars and required monthly saving<br><img src="docs/img/screens/09_savings_goals.png" width="480"> |
| **EMI calculator** - estimate without applying<br><img src="docs/img/screens/10_emi_calculator.png" width="480"> | **Amortization table** - month-by-month interest and principal<br><img src="docs/img/screens/11_amortization.png" width="480"> |
| **Interest & FD simulator** - same amount in each account type<br><img src="docs/img/screens/12_simulator.png" width="480"> | **Home screen**<br><img src="docs/img/screens/01_home.png" width="480"> |

### Security and fraud

| | |
|---|---|
| **PIN lockout** after 3 wrong attempts (masked PIN entry)<br><img src="docs/img/screens/14_lockout.png" width="480"> | **Fraud alerts** raised by the polymorphic rules<br><img src="docs/img/screens/13_fraud_alerts.png" width="480"> |
| **FD early-withdrawal penalty** shown before the money moves<br><img src="docs/img/screens/15_fd_penalty.png" width="480"> | |

### Admin

| | |
|---|---|
| **Admin dashboard** - pending loans, alerts, locked users<br><img src="docs/img/screens/16_admin_dashboard.png" width="480"> | **Open account** - creates the account and the customer login<br><img src="docs/img/screens/17_admin_open.png" width="480"> |
| **Loan approvals**<br><img src="docs/img/screens/18_admin_loans.png" width="480"> | **Fraud alert review**<br><img src="docs/img/screens/19_admin_fraud.png" width="480"> |
| **Reports** - richest accounts chart<br><img src="docs/img/screens/20_admin_richest.png" width="480"> | **Append-only audit log**<br><img src="docs/img/screens/21_admin_audit.png" width="480"> |

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

## Quick start

Requirements: a C++17 compiler (g++ 9+ or clang 10+) and `make`. No external libraries.

```bash
cd BankingSystem
make            # builds ./bank
./bank          # first run creates ./data with demo accounts
```

```text
./bank --ascii        plain ASCII boxes (for terminals without Unicode)
./bank --no-color     disable ANSI colors
./bank --data DIR     use a different data folder (or set BANK_DATA_DIR)
BANK_TIMEOUT_SEC=60   change the idle auto-logout (default 300 s)
```

Other targets: `make test`, `make widths`, `make valgrind`, `make check` (all three), `make screenshots`, `make diagrams`, `make clean`.

Developed and tested on Linux with g++ 13. The code guards the few OS-specific parts (masked PIN input, console UTF-8/VT mode, `localtime`) for Windows, but it has not been run there yet; on Windows use a UTF-8 capable terminal such as Windows Terminal.

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

## Web version (runs on localhost)

The same Bank engine is also available as a **browser app**. A small C++ server (`web/server.cpp`, built on the single-header [cpp-httplib](https://github.com/yhirose/cpp-httplib)) exposes the `Bank` classes as a JSON API and serves a plain HTML/CSS/JavaScript front end. No Node, no frameworks, no database: it uses the same CSV files as the console app, so both versions share data.

<p align="center"><img src="docs/img/web_architecture.png" width="900"></p>

### Run it

```bash
cd BankingSystem
make bankweb          # builds ./bankweb (about 15 s the first time)
./bankweb             # then open http://localhost:8080
```

```text
./bankweb --port 8090      use another port
./bankweb --data DIR       use another data folder (or BANK_DATA_DIR)
BANK_TIMEOUT_SEC=300       idle session timeout (default 600 s)
```

On **Windows (MinGW)** run `mingw32-make bankweb` then `.\bankweb.exe`. Press `Ctrl+C` to stop the server. The Windows build of the web server has not been tested by me (the Linux build has); if it fails, WSL is the safe route.

Log in with the same demo accounts (`SB1001`/`1234`, `CA2001`/`2345`, `FD3001`/`3456`, `ST1001`/`4567`, admin `admin`/`admin123`). The login page has click-to-fill chips.

### What the web UI adds

- Dashboard with balance card, balance trend, monthly in/out and recent activity.
- Confirmation dialogs before money moves (the browser version of the console Y/N guard), live recipient-name lookup, printable receipts.
- Fraud alerts pop up as a toast the moment a transaction triggers a rule, with an unread badge in the sidebar.
- Statement filters, sorting, running balance and `.txt` export; spending bars; goal progress bars; EMI calculator with live results and amortization table; interest simulator with a line chart.
- Full admin console: account search/open/rename/close, loan approvals, fraud review, unlock users, reports and the audit log.

### Screenshots (captured from the running web app)

| | |
|---|---|
| **Login**<br><img src="docs/img/web/01_login.png" width="480"> | **Dashboard**<br><img src="docs/img/web/03_dashboard.png" width="480"> |
| **Receipt** after a deposit<br><img src="docs/img/web/04_receipt.png" width="480"> | **Transfer confirmation**<br><img src="docs/img/web/05_transfer_confirm.png" width="480"> |
| **Smart statement**<br><img src="docs/img/web/06_statement.png" width="480"> | **Spending categories**<br><img src="docs/img/web/07_spending.png" width="480"> |
| **Savings goals**<br><img src="docs/img/web/08_goals.png" width="480"> | **Loans and EMI calculator**<br><img src="docs/img/web/09_loans_emi.png" width="480"> |
| **Amortization table**<br><img src="docs/img/web/10_amortization.png" width="480"> | **Interest simulator**<br><img src="docs/img/web/12_simulator.png" width="480"> |
| **Fraud alert raised live**<br><img src="docs/img/web/13_fraud_toast.png" width="480"> | **Security and alerts**<br><img src="docs/img/web/14_security_alerts.png" width="480"> |
| **PIN lockout**<br><img src="docs/img/web/15_locked.png" width="480"> | **FD penalty shown before withdrawing**<br><img src="docs/img/web/16_fd_penalty.png" width="480"> |
| **Admin overview**<br><img src="docs/img/web/17_admin_overview.png" width="480"> | **Admin accounts**<br><img src="docs/img/web/18_admin_accounts.png" width="480"> |
| **Loan approvals**<br><img src="docs/img/web/19_admin_loans.png" width="480"> | **Fraud review**<br><img src="docs/img/web/20_admin_fraud.png" width="480"> |
| **Reports**<br><img src="docs/img/web/22_admin_reports.png" width="480"> | **Audit log**<br><img src="docs/img/web/23_admin_audit.png" width="480"> |

Regenerate them with `make bankweb && tools/run_web_shots.sh` (needs Node and Chrome; it starts a throw-away server with fresh demo data).

### API at a glance

All endpoints are JSON under `/api`. Errors come back as `{"error": "message"}` with a matching status (400 validation, 401 not logged in or wrong PIN, 403 wrong role, 423 locked).

| Area | Endpoints |
|---|---|
| Session | `POST /login`, `POST /logout`, `GET /me` |
| Money | `POST /deposit`, `POST /withdraw`, `POST /transfer`, `GET /fdfee`, `GET /lookup`, `GET /receipt` |
| Insight | `GET /dashboard`, `GET /statement`, `GET /statement.txt`, `GET /spending`, `GET /simulate` |
| Goals, loans, payees | `GET/POST /goals`, `POST /goals/contribute`, `GET /emi`, `GET/POST /loans`, `/loans/apply`, `/loans/schedule`, `/loans/pay`, `/loans/prepay`, `GET/POST /payees`, `POST /payees/remove` |
| Security | `GET /alerts`, `POST /alerts/seen`, `POST /pin` |
| Admin | `/admin/overview`, `/admin/accounts` (+ `update`, `close`), `/admin/history`, `/admin/loans` (+ `approve`, `reject`), `/admin/alerts` (+ `review`), `/admin/locked`, `/admin/unlock`, `/admin/interest`, `/admin/reports`, `/admin/audit` |

### Web security notes

- Listens on `127.0.0.1` only, so other machines cannot reach it. Requests whose `Host` header is not localhost are rejected (DNS-rebinding guard).
- Session cookie is `HttpOnly` and `SameSite=Strict`; `POST` requires `Content-Type: application/json`, which a cross-site form cannot send.
- The UI builds the page with `textContent` instead of `innerHTML`, and a Content-Security-Policy blocks inline scripts and outside resources.
- Every ownership check (your own loans, goals, receipts) is enforced in the server, not just hidden in the UI.
- It is a learning project: plain HTTP, a single process serialized by one mutex, and the educational PIN hash. Do not expose it to the internet.

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

The scripted sessions in [`scenarios/`](scenarios) drive the real binary end to end and are also the source of the screenshots.

## Project structure

```text
BankingSystem/
├── include/        headers (Account, User, Loan, Fraud, Bank, Repository<T>, ConsoleUI, App ...)
├── src/            implementations + main.cpp
├── tests/          7 test suites + a tiny assert harness
├── scenarios/      scripted demo sessions (customer, admin, lockout and fraud)
├── web/            localhost web version: server.cpp, public/ (HTML, CSS, JS), third_party/httplib.h
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
- **Windows** support is written in but untested (see Quick start).

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
| `web-ui` | Nova Bank plus a version that runs in the browser on localhost |

See the `main` branch README for an overview of all ten experiments.

## Author

Built by **Jehal05** as the OOP lab mini project. Experiments 1-10 live on the `experiment-01` ... `experiment-10` branches of this repository.
