// Nova Bank - a menu driven banking program (C++17, console).
// main.cpp only handles menus, input and printing. All rules live in the Bank class.
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include "Bank.h"
#include "Exceptions.h"
#include "Utils.h"

using namespace std;

struct EndOfInput {};     // thrown when the input stream closes (Ctrl+Z / Ctrl+D)

// ---------------------------------------------------------------- input helpers
string readLine(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) throw EndOfInput{};
    return util::trim(s);
}

int readInt(const string& prompt, int lo, int hi) {
    for (;;) {
        string s = readLine(prompt);
        char* end = nullptr;
        long v = strtol(s.c_str(), &end, 10);
        if (!s.empty() && *end == '\0' && v >= lo && v <= hi) return (int)v;
        cout << "Please enter a number between " << lo << " and " << hi << ".\n";
    }
}

double readAmount(const string& prompt) {
    for (;;) {
        string s = readLine(prompt);
        char* end = nullptr;
        double v = strtod(s.c_str(), &end);
        if (!s.empty() && *end == '\0' && v > 0 && v <= 1e9) return v;
        cout << "Please enter an amount greater than zero.\n";
    }
}

bool confirm(const string& prompt) {
    for (;;) {
        string s = util::lower(readLine(prompt + " (y/n): "));
        if (s == "y" || s == "yes") return true;
        if (s == "n" || s == "no") return false;
    }
}

void heading(const string& text) {
    cout << "\n==== " << text << " ====\n";
}

// ---------------------------------------------------------------- printing helpers
void printTransactions(const vector<Transaction>& list) {
    if (list.empty()) { cout << "No transactions found.\n"; return; }
    cout << left << setw(17) << "Date" << setw(20) << "Reference" << setw(14) << "Type" << right << setw(16) << "Amount"
         << setw(16) << "Balance" << "  Note\n";
    cout << string(95, '-') << "\n";
    for (const auto& t : list)
        cout << left << setw(17) << util::formatTime(t.time) << setw(20) << t.ref << setw(14) << t.type << right
             << setw(16) << ((t.credit ? "+" : "-") + util::money(t.amount)) << setw(16) << util::money(t.balanceAfter)
             << "  " << t.note << "\n";
}

void printLoan(const Loan& l) {
    cout << l.id() << "  " << l.typeName() << "  " << util::money(l.principal()) << "  " << l.months() << " months @ "
         << l.annualRate() << "%  EMI " << util::money(l.emi()) << "  [" << toString(l.status()) << "]";
    if (l.status() == LoanStatus::Active) cout << "  paid " << l.paid() << "/" << l.months();
    cout << "\n";
}

void printSchedule(const Loan& l) {
    cout << left << setw(8) << "Month" << right << setw(14) << "EMI" << setw(14) << "Interest" << setw(14) << "Principal"
         << setw(16) << "Balance" << "\n" << string(66, '-') << "\n";
    for (const auto& r : l.schedule()) {
        cout << left << setw(8) << (to_string(r.month) + (r.month <= l.paid() ? "*" : "")) << right << fixed << setprecision(2)
             << setw(14) << r.emi << setw(14) << r.interest << setw(14) << r.principal << setw(16) << r.balance << "\n";
    }
    if (l.paid() > 0) cout << "(* = already paid)\n";
}

// ---------------------------------------------------------------- customer menus
void statementMenu(Bank& bank, Account& acc) {
    for (;;) {
        heading("Statement");
        cout << "1. All transactions\n2. Last 10 transactions\n3. Only one type (deposit, withdraw, ...)\n"
                "4. Only large transactions (minimum amount)\n5. Save to a text file\n0. Back\n";
        int c = readInt("Choose: ", 0, 5);
        if (c == 0) return;
        if (c == 1) printTransactions(bank.history(acc.accNo()));
        else if (c == 2) {
            auto h = bank.history(acc.accNo());
            if (h.size() > 10) h.erase(h.begin(), h.end() - 10);
            printTransactions(h);
        } else if (c == 3) {
            cout << "Types: Deposit, Withdraw, Transfer Out, Transfer In, Interest, Loan Credit, EMI, Fee\n";
            printTransactions(bank.statement(acc.accNo(), readLine("Type: "), 0));
        } else if (c == 4) {
            printTransactions(bank.statement(acc.accNo(), "", readAmount("Minimum amount (Rs): ")));
        } else {
            cout << "Saved to " << bank.exportStatement(acc.accNo()) << "\n";
        }
    }
}

void loanMenu(Bank& bank, Account& acc) {
    for (;;) {
        heading("Loans");
        auto mine = bank.loansOf(acc.accNo());
        for (Loan* l : mine) printLoan(*l);
        if (mine.empty()) cout << "You have no loans.\n";
        cout << "\n1. Apply for a loan\n2. EMI calculator\n3. Pay next EMI\n4. Show repayment schedule\n0. Back\n";
        int c = readInt("Choose: ", 0, 4);
        if (c == 0) return;
        if (c == 1 || c == 2) {
            cout << "Loan types: 1 Home (8.5%, up to Rs 50,00,000, 240 months)\n"
                    "            2 Education (9.5%, up to Rs 15,00,000, 120 months)\n"
                    "            3 Personal (12.5%, up to Rs 5,00,000, 60 months)\n";
            const char* codes[] = {"HOME", "EDU", "PER"};
            string code = codes[readInt("Type: ", 1, 3) - 1];
            double amount = readAmount("Loan amount (Rs): ");
            int months = readInt("Tenure in months: ", 1, 240);
            auto preview = Loan::create(code, "TMP", acc.accNo(), amount, months);
            if (amount > preview->maxAmount() || months > preview->maxMonths()) {
                cout << "Outside the limit for a " << preview->typeName() << ".\n";
                continue;
            }
            double total = 0;
            for (const auto& r : preview->schedule()) total += r.emi;
            cout << "Monthly EMI: " << util::money(preview->emi()) << "   Total payable: " << util::money(total)
                 << "   Total interest: " << util::money(total - amount) << "\n";
            if (c == 1 && confirm("Send this application to the bank?")) {
                Loan& l = bank.applyLoan(acc.accNo(), code, amount, months);
                cout << "Application " << l.id() << " submitted. The admin will approve or reject it.\n";
            }
        } else {
            string id = readLine("Loan ID: ");
            Loan* chosen = nullptr;
            for (Loan* l : mine) if (l->id() == util::upper(id)) chosen = l;
            if (!chosen) { cout << "That is not one of your loans.\n"; continue; }
            if (c == 4) printSchedule(*chosen);
            else if (confirm("Pay " + util::money(chosen->nextInstallment()) + " now?")) {
                Transaction t = bank.payEmi(chosen->id(), acc.accNo());
                cout << "EMI paid. Reference " << t.ref << ". Balance " << util::money(acc.balance()) << "\n";
            }
        }
    }
}

void alertsMenu(Bank& bank, Account& acc) {
    heading("Security alerts");
    auto list = bank.alertsOf(acc.accNo());
    if (list.empty()) cout << "No alerts. Everything looks normal.\n";
    for (Alert* a : list)
        cout << (a->seen ? "  " : "* ") << a->id << "  " << util::formatTime(a->time) << "  " << a->rule << ": " << a->message << "\n";
    if (!list.empty()) cout << "(* = new)\n";
    bank.markAlertsSeen(acc.accNo());
}

void customerMenu(Bank& bank, Account& acc) {
    heading("Welcome, " + acc.name());
    int unread = 0;
    for (Alert* a : bank.alertsOf(acc.accNo())) if (!a->seen) ++unread;
    if (unread) cout << "!! You have " << unread << " new security alert(s). Choose option 7 to read them.\n";
    for (;;) {
        heading("Customer menu - " + acc.accNo());
        cout << "1. Account details\n2. Deposit\n3. Withdraw\n4. Transfer\n5. Statement\n6. Loans\n7. Security alerts\n"
                "8. Change PIN\n0. Logout\n";
        int c = readInt("Choose: ", 0, 8);
        try {
            if (c == 0) { bank.audit(acc.accNo(), "LOGOUT", ""); return; }
            if (c == 1) {
                cout << acc << "\nRules: " << acc.rulesText() << "\nAvailable to withdraw: " << util::money(acc.available())
                     << "\nOpened on " << util::formatDate(acc.opened()) << "\n";
            } else if (c == 2) {
                Transaction t = bank.deposit(acc.accNo(), readAmount("Amount to deposit (Rs): "), "Deposit");
                cout << "Deposited. Reference " << t.ref << ". New balance " << util::money(acc.balance()) << "\n";
            } else if (c == 3) {
                double amount = readAmount("Amount to withdraw (Rs): ");
                double fee = acc.withdrawalFee(amount);
                if (fee > 0) cout << "Early withdrawal penalty: " << util::money(fee) << "\n";
                if (confirm("Withdraw " + util::money(amount) + "?")) {
                    Transaction t = bank.withdraw(acc.accNo(), amount, "Withdrawal");
                    cout << "Done. Reference " << t.ref << ". New balance " << util::money(acc.balance()) << "\n";
                }
            } else if (c == 4) {
                string to = util::upper(readLine("Recipient account number: "));
                Account* dst = bank.find(to);
                if (!dst) { cout << "No such account.\n"; continue; }
                double amount = readAmount("Amount to transfer (Rs): ");
                string note = readLine("Note (optional): ");
                if (confirm("Send " + util::money(amount) + " to " + dst->name() + " (" + dst->accNo() + ")?")) {
                    Transaction t = bank.transfer(acc.accNo(), to, amount, note);
                    cout << "Sent. Reference " << t.ref << ". New balance " << util::money(acc.balance()) << "\n";
                }
            } else if (c == 5) statementMenu(bank, acc);
            else if (c == 6) loanMenu(bank, acc);
            else if (c == 7) alertsMenu(bank, acc);
            else {
                string oldPin = readLine("Current PIN: ");
                string newPin = readLine("New PIN (4 digits): ");
                bank.changePin(acc, oldPin, newPin);
                cout << "PIN changed.\n";
            }
        } catch (const BankException& e) {
            cout << "Error: " << e.what() << "\n";
        }
    }
}

// ---------------------------------------------------------------- admin menu
void adminMenu(Bank& bank) {
    for (;;) {
        heading("Admin menu");
        cout << "1. Open a new account\n2. List all accounts\n3. View an account's transactions\n4. Pending loans\n"
                "5. Fraud alerts to review\n6. Locked accounts\n7. Pay monthly interest\n8. Audit log\n0. Logout\n";
        int c = readInt("Choose: ", 0, 8);
        try {
            if (c == 0) return;
            if (c == 1) {
                cout << "Types: 1 Savings  2 Student  3 Current  4 Fixed Deposit\n";
                const char* codes[] = {"SAV", "STU", "CUR", "FD"};
                string code = codes[readInt("Type: ", 1, 4) - 1];
                string name = readLine("Customer name: ");
                double deposit = readAmount("Opening deposit (Rs): ");
                string pin = readLine("4-digit PIN for the customer: ");
                string no = bank.openAccount(code, name, deposit, pin);
                cout << "Account opened: " << no << " (the customer logs in with this number and the PIN)\n";
            } else if (c == 2) {
                for (const auto& a : bank.accounts()) cout << *a << (a->locked() ? "  [LOCKED]" : "") << "\n";
            } else if (c == 3) {
                printTransactions(bank.history(util::upper(readLine("Account number: "))));
            } else if (c == 4) {
                auto pending = bank.pendingLoans();
                if (pending.empty()) { cout << "No pending loans.\n"; continue; }
                for (Loan* l : pending) cout << l->accNo() << ": ", printLoan(*l);
                string id = readLine("Loan ID to decide (Enter to go back): ");
                if (id.empty()) continue;
                cout << "1. Approve  2. Reject\n";
                if (readInt("Choose: ", 1, 2) == 1) { bank.approveLoan(id); cout << "Loan approved and credited.\n"; }
                else { bank.rejectLoan(id); cout << "Loan rejected.\n"; }
            } else if (c == 5) {
                auto list = bank.alertsToReview();
                if (list.empty()) { cout << "Nothing to review.\n"; continue; }
                for (Alert* a : list) cout << a->id << "  " << a->accNo << "  " << util::formatTime(a->time) << "  " << a->rule << ": " << a->message << "\n";
                string id = readLine("Alert ID to mark as reviewed (Enter to go back): ");
                if (!id.empty()) { bank.reviewAlert(id); cout << "Marked as reviewed.\n"; }
            } else if (c == 6) {
                auto locked = bank.lockedAccounts();
                if (locked.empty()) { cout << "No locked accounts.\n"; continue; }
                for (Account* a : locked) cout << *a << "\n";
                string no = readLine("Account to unlock (Enter to go back): ");
                if (!no.empty()) { bank.unlock(no); cout << "Account unlocked.\n"; }
            } else if (c == 7) {
                if (confirm("Pay one month of interest to every account?"))
                    cout << "Total interest paid: " << util::money(bank.applyMonthlyInterest()) << "\n";
            } else {
                for (const auto& line : bank.auditTail(20)) cout << line << "\n";
            }
        } catch (const BankException& e) {
            cout << "Error: " << e.what() << "\n";
        }
    }
}

// ---------------------------------------------------------------- main
int main() {
    string dataDir = getenv("BANK_DATA_DIR") ? getenv("BANK_DATA_DIR") : "data";
    Bank bank(dataDir);
    try {
        bank.load();
        cout << "NOVA BANK - Banking Management System\n";
        cout << "Demo logins: customer SB1001/1234, CA2001/2345, FD3001/3456, ST1001/4567 | admin admin/admin123\n";
        for (;;) {
            heading("Main menu");
            cout << "1. Customer login\n2. Admin login\n0. Exit\n";
            int c = readInt("Choose: ", 0, 2);
            try {
                if (c == 0) break;
                if (c == 1) {
                    string no = readLine("Account number: ");
                    string pin = readLine("PIN: ");
                    Account& acc = bank.login(no, pin);
                    customerMenu(bank, acc);
                } else {
                    string id = readLine("Admin ID: ");
                    string pw = readLine("Password: ");
                    if (bank.adminLogin(id, pw)) adminMenu(bank);
                    else cout << "Wrong admin ID or password.\n";
                }
            } catch (const BankException& e) {
                cout << "Error: " << e.what() << "\n";
            }
        }
        cout << "Goodbye.\n";
    } catch (const EndOfInput&) {
        cout << "\nInput closed. Goodbye.\n";
    } catch (const exception& e) {
        cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
