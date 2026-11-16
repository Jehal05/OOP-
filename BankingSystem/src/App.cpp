// App.cpp - shared helpers, login screen and the customer experience.
#include "App.h"
#include "Utils.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

using ui::Kind;
using ui::Align;
namespace c = ui::c;

void printChoices(const std::string& label, const std::vector<std::string>& names, size_t perLine) {
    for (size_t i = 0; i < names.size(); i += perLine) {
        std::string row = i == 0 ? " " + label + " " : std::string(label.size() + 2, ' ');
        for (size_t j = i; j < std::min(names.size(), i + perLine); ++j)
            row += c::cyan() + "[" + std::to_string(j + 1) + "]" + c::reset() + " " + ui::padRight(names[j], 12);
        while (!row.empty() && row.back() == ' ') row.pop_back();
        ui::line(row);
    }
}

// ============================================================ construction / shared helpers
App::App(Bank& bank) : bank_(bank), timeoutSec_(300) {
    std::filesystem::path parent = std::filesystem::path(bank.dataDir()).parent_path();
    stmtDir_ = (parent / "statements").string();
    receiptDir_ = (parent / "receipts").string();
    if (const char* t = std::getenv("BANK_TIMEOUT_SEC")) timeoutSec_ = std::max(1, std::atoi(t));
    touch();
}

void App::checkSession() const {
    auto idle = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - lastActivity_).count();
    if (idle > timeoutSec_) throw SessionExpired{};
}

int App::choose(const std::vector<MenuItem>& items, const std::string& prompt) {
    std::vector<std::string> lines;
    for (const auto& it : items) {
        std::string key = c::bold() + c::cyan() + "[" + std::to_string(it.key) + "]" + c::reset();
        lines.push_back(key + " " + ui::padRight(it.label, 26) + c::dim() + it.desc + c::reset());
    }
    ui::box("Menu", lines);
    for (;;) {
        int k = ui::readInt(prompt, 0, 9);
        checkSession();                                           // auto-logout after inactivity
        touch();
        for (const auto& it : items) if (it.key == k) return k;
        ui::banner(Kind::Error, "That option is not available here.");
    }
}

void App::showError(const BankException& e) { ui::banner(Kind::Error, e.what()); }

Category App::pickCategory() {
    std::vector<std::string> names;
    for (Category cat : allCategories()) names.push_back(toString(cat));
    printChoices("Category:", names, 4);
    return allCategories()[(size_t)ui::readInt("Choose category", 1, 8, 8) - 1];
}

std::string App::amountCell(const Transaction& t) const {
    std::string txt = (t.credit() ? "+" : "-") + util::money(t.amount());
    return ui::paint(t.credit() ? c::green() : c::red(), txt);
}

void App::receipt(const Transaction& tx, bool save) {
    const Account* a = bank_.findAccount(tx.accNo());
    std::vector<std::string> kv = {
        "Reference", tx.refId(), "Date", util::fmtDateTime(tx.timestamp()),
        "Account", util::maskAccount(tx.accNo()) + (a ? " (" + a->holder() + ")" : ""),
        "Type", toString(tx.type()), "Category", tx.credit() ? "-" : toString(tx.category()),
        "Amount", (tx.credit() ? "+" : "-") + util::money(tx.amount()),
        "Balance after", util::money(tx.balanceAfter()), "Note", tx.note().empty() ? "-" : tx.note()};
    std::vector<std::string> lines;
    for (size_t i = 0; i < kv.size(); i += 2) {
        std::string val = kv[i + 1];
        if (kv[i] == "Amount") val = ui::paint(tx.credit() ? c::green() : c::red(), val);
        lines.push_back(c::dim() + ui::padRight(kv[i], 14) + c::reset() + val);
    }
    ui::blank();
    ui::box("TRANSACTION RECEIPT", lines, 60);
    if (save) {
        try {
            std::filesystem::create_directories(receiptDir_);
            std::string path = receiptDir_ + "/" + tx.refId() + ".txt";
            std::ofstream out(path, std::ios::binary | std::ios::trunc);
            out << "NOVA BANK - TRANSACTION RECEIPT\n" << std::string(44, '=') << "\n";
            for (size_t i = 0; i < kv.size(); i += 2) {
                std::string k = kv[i];
                k.resize(14, ' ');
                out << k << kv[i + 1] << "\n";
            }
            out << std::string(44, '=') << "\nThank you for banking with Nova Bank.\n";
            ui::line(c::dim() + "  Receipt saved: " + path + c::reset());
        } catch (const std::exception&) { ui::banner(Kind::Warning, "Receipt could not be saved to disk."); }
    }
}

void App::accountCard(const Account& a) {
    std::vector<std::string> lines = {
        c::bold() + a.holder() + c::reset() + "   " + c::dim() + util::maskAccount(a.accNo()) + "  |  " + a.typeName() + c::reset(),
        "Available balance   " + c::bold() + c::green() + util::money(a.balance()) + c::reset() +
            (a.balance() < 0 ? c::red() + "  (overdrawn)" + c::reset() : ""),
        c::dim() + a.describe() + c::reset()};
    ui::box("Account", lines);
}

bool App::askDate(const std::string& prompt, std::optional<time_t>& out) {
    for (;;) {
        std::string s = ui::readLine(prompt + c::dim() + " (YYYY-MM-DD, Enter to skip)" + c::reset());
        if (s.empty()) { out.reset(); return true; }
        time_t t;
        if (util::parseDate(s, t)) { out = t; return true; }
        ui::banner(Kind::Error, "Use the format YYYY-MM-DD, for example 2026-11-01.");
    }
}

StatementFilter App::askFilter() {
    StatementFilter f;
    ui::title("Filters  (press Enter to skip any)");
    askDate("From date", f.from);
    askDate("To date  ", f.to);
    std::vector<std::string> tnames;
    for (TxType t : allTxTypes()) tnames.push_back(toString(t));
    printChoices("Types:", tnames, 4);
    int ti = ui::readInt("Transaction type (0 = any)", 0, (int)allTxTypes().size(), 0);
    if (ti > 0) f.type = allTxTypes()[(size_t)ti - 1];
    std::vector<std::string> cnames;
    for (Category cat : allCategories()) cnames.push_back(toString(cat));
    printChoices("Categories:", cnames, 4);
    int ci = ui::readInt("Category (0 = any)", 0, 8, 0);
    if (ci > 0) f.category = allCategories()[(size_t)ci - 1];
    f.minAmount = ui::readOptionalAmount("Minimum amount");
    f.maxAmount = ui::readOptionalAmount("Maximum amount");
    return f;
}

void App::showStatementTable(const std::vector<Transaction>& rows, size_t limit) {
    std::vector<std::vector<std::string>> body;
    size_t shown = std::min(rows.size(), limit);                        // first rows in the order the user asked for
    for (size_t i = 0; i < shown; ++i) {
        const Transaction& t = rows[i];
        body.push_back({util::fmtShort(t.timestamp()), toString(t.type()), t.credit() ? "-" : toString(t.category()),
                        amountCell(t), util::money(t.balanceAfter())});
    }
    if (body.empty()) { ui::banner(Kind::Info, "No transactions match your filters."); return; }
    ui::table({"Date", "Type", "Category", "Amount", "Balance"}, body,
              {Align::Left, Align::Left, Align::Left, Align::Right, Align::Right});
    if (rows.size() > shown) ui::line(c::dim() + " Showing " + std::to_string(shown) + " of " + std::to_string(rows.size()) +
                                      " rows. Export to .txt for the full list." + c::reset());
}

// One reusable "smart statement" workflow, used by customers and by admins (for any account).
void App::statementFlow(const std::string& accNo, const std::string& crumb) {
    const Account& acc = bank_.account(accNo);
    for (;;) {
        ui::screen(crumb);
        int k = choose({{1, "View statement", "filters, sorting, running balance"},
                        {2, "Monthly summary", "totals grouped by month"},
                        {3, "Export to .txt", "bank-style statement file"},
                        {4, "Find by reference ID", "look up one transaction"},
                        {0, "Back", ""}});
        if (k == 0) return;
        try {
            if (k == 4) {
                std::string ref = ui::readLine("Reference ID (e.g. TXN-20261116-000123)");
                const Transaction* t = bank_.findTransaction(ref);
                if (!t || t->accNo() != accNo) ui::banner(Kind::Error, "No transaction with that reference in this account.");
                else receipt(*t, false);
                ui::pause();
                continue;
            }
            StatementFilter f;
            SortKey key = SortKey::Date;
            bool asc = false;
            if (k != 2) {
                f = askFilter();
                key = ui::readInt("Sort by  [1] Date  [2] Amount", 1, 2, 1) == 1 ? SortKey::Date : SortKey::Amount;
                asc = ui::readInt("Order    [1] Newest/Largest first  [2] Oldest/Smallest first", 1, 2, 1) == 2;
            }
            Statement st(accNo, acc.holder(), bank_.history(accNo));
            st.filter(f);
            if (k == 2) {
                std::vector<std::vector<std::string>> body;
                for (const auto& [month, rows] : st.byMonth()) {
                    double cr = 0, db = 0;
                    for (const auto& t : rows) (t.credit() ? cr : db) += t.amount();
                    double net = cr - db;
                    body.push_back({month, std::to_string(rows.size()), ui::paint(c::green(), util::money(cr)),
                                    ui::paint(c::red(), util::money(db)),
                                    ui::paint(net >= 0 ? c::green() : c::red(), (net >= 0 ? "+" : "") + util::money(net))});
                }
                ui::screen(crumb + " > Monthly summary");
                if (body.empty()) ui::banner(Kind::Info, "No transactions yet.");
                else ui::table({"Month", "Txns", "Credits", "Debits", "Net"}, body,
                               {Align::Left, Align::Right, Align::Right, Align::Right, Align::Right});
                ui::pause();
                continue;
            }
            if (k == 3) {
                st.sort(key, asc);
                ui::banner(Kind::Info, "Generating statement" + std::string(ui::unicodeEnabled() ? "…" : "..."));
                std::string path = st.exportText(stmtDir_, f, acc.balance());
                bank_.logAudit(accNo, "STATEMENT_EXPORTED", path);
                ui::banner(Kind::Success, "Statement saved to " + path);
                ui::pause();
                continue;
            }
            st.sort(key, asc);
            ui::screen(crumb + " > Statement");
            ui::line(c::dim() + " Account " + util::maskAccount(accNo) + "  |  " + acc.holder() + "  |  Filters: " + f.describe() + c::reset());
            ui::blank();
            std::vector<Transaction> rows = st.rows();
            showStatementTable(rows, 20);
            StatementSummary s = st.summary();
            ui::blank();
            ui::box("Summary", {
                "Transactions  " + std::to_string(s.count),
                "Total credits " + ui::paint(c::green(), util::money(s.credits)) + "    Total debits " + ui::paint(c::red(), util::money(s.debits)),
                "Net change    " + ui::paint(s.net >= 0 ? c::green() : c::red(), (s.net >= 0 ? "+" : "") + util::money(s.net)),
                "Average       " + util::money(s.average) + "    Largest " + util::money(s.largest)});
            ui::pause();
        } catch (const BankException& e) { showError(e); ui::pause(); }
    }
}

void App::loanTable(const std::vector<Loan*>& loans, bool withAccount) {
    std::vector<std::vector<std::string>> body;
    auto shortType = [](std::string t) { auto p = t.find(" Loan"); return p == std::string::npos ? t : t.substr(0, p); };
    for (const Loan* l : loans) {
        std::string col = l->status() == LoanStatus::Active ? c::green() : (l->status() == LoanStatus::Pending ? c::yellow() :
                          (l->status() == LoanStatus::Rejected ? c::red() : c::dim()));
        std::vector<std::string> row = {l->id()};
        if (withAccount) row.push_back(l->accNo());
        row.insert(row.end(), {shortType(l->typeName()), util::moneyShort(l->principal()), util::number(l->annualRate(), 1) + "%",
                               std::to_string(l->months()) + "m", ui::paint(col, toString(l->status()))});
        if (!withAccount) {
            row.push_back(std::to_string(l->paid()) + "/" + std::to_string(l->months()));
            row.push_back(util::moneyShort(l->outstanding()));
        }
        body.push_back(row);
    }
    std::vector<std::string> hdr = {"ID"};
    if (withAccount) hdr.push_back("Account");
    hdr.insert(hdr.end(), {"Type", "Amount", "Rate", "Term", "Status"});
    if (!withAccount) hdr.insert(hdr.end(), {"Paid", "Owed"});
    std::vector<Align> al(hdr.size(), Align::Left);
    for (size_t i = withAccount ? 3 : 2; i < hdr.size(); ++i) al[i] = Align::Right;
    al[withAccount ? 6 : 5] = Align::Left;
    ui::table(hdr, body, al);
}

void App::amortizationTable(const Loan& loan) {
    auto sch = loan.schedule();
    int start = ui::readInt("Start at month", 1, loan.months(), std::min(loan.months(), loan.paid() + 1));
    std::vector<std::vector<std::string>> body;
    for (int i = start; i < start + 12 && i <= loan.months(); ++i) {
        const Installment& r = sch[(size_t)i - 1];
        bool paid = loan.status() == LoanStatus::Active && i <= loan.paid();
        body.push_back({std::to_string(r.month) + (paid ? (ui::unicodeEnabled() ? " ✔" : " *") : ""), util::money(r.emi),
                        util::money(r.interest), util::money(r.principal), util::money(r.balance)});
    }
    double totalInterest = 0, totalPay = 0;
    for (const auto& r : sch) { totalInterest += r.interest; totalPay += r.emi; }
    ui::blank();
    ui::table({"Month", "EMI", "Interest", "Principal", "Balance"}, body,
              {Align::Left, Align::Right, Align::Right, Align::Right, Align::Right});
    ui::line(c::dim() + " Total interest " + util::money(totalInterest) + "   Total payable " + util::money(totalPay) + c::reset());
}

void App::emiPreview(const Loan& probe) {
    double totalPay = 0;
    for (const auto& r : probe.schedule()) totalPay += r.emi;               // same figures as the amortization table
    ui::box("EMI Estimate", {
        probe.typeName() + "   " + c::dim() + "@ " + util::number(probe.annualRate(), 1) + "% p.a." + c::reset(),
        "Loan amount   " + util::money(probe.principal()) + "    Tenure " + std::to_string(probe.months()) + " months",
        "Monthly EMI   " + c::bold() + c::cyan() + util::money(probe.emi()) + c::reset(),
        "Total interest " + util::money(totalPay - probe.principal()) + "    Total payable " + util::money(totalPay)});
}

// ============================================================ main loop / login
void App::run() {
    for (;;) {
        ui::screen("Home");
        ui::box("Welcome", {
            "Secure banking for customers and staff.",
            c::dim() + "Demo logins:  customer SB1001 / 1234   |   admin  admin / admin123" + c::reset()});
        ui::blank();
        int k = choose({{1, "Customer login", "accounts, payments, loans, goals"},
                        {2, "Admin login", "manage accounts, loans and security"},
                        {3, "Exit", "save and quit"}}, "Choose an option");
        // choose() accepts 0-9 but only listed keys are returned
        if (k == 3) { bank_.saveAll(); ui::banner(Kind::Info, "Data saved. Thank you for banking with Nova Bank."); return; }
        std::string role = k == 1 ? "CUSTOMER" : "ADMIN";
        ui::screen(std::string("Home > ") + (k == 1 ? "Customer login" : "Admin login"));
        try {
            std::string id = ui::readLine(k == 1 ? "Account number" : "Admin ID");
            std::string pin = ui::readPin(k == 1 ? "PIN" : "Password");
            User& u = bank_.login(id, pin, role);
            touch();
            try {
                if (k == 1) customerSession(u); else adminSession(u);
            } catch (const SessionExpired&) {
                ui::screen("Home");
                ui::banner(Kind::Warning, "Session expired after " + std::to_string(timeoutSec_) + "s of inactivity. You were logged out for your safety.");
                bank_.logAudit(u.id(), "SESSION_TIMEOUT");
            }
            bank_.logAudit(u.id(), "LOGOUT");
            ui::banner(Kind::Success, "You have been logged out.");
        } catch (const AccountLockedException& e) {
            showError(e);
        } catch (const AuthenticationException& e) {
            std::string extra = e.attemptsLeft() >= 0 ? " Attempts remaining: " + std::to_string(e.attemptsLeft()) + "." : "";
            ui::banner(Kind::Error, std::string(e.what()) + extra);
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

// ============================================================ customer
void App::customerDashboard(const Customer& cu) {
    const Account& a = bank_.account(cu.linkedAccount());
    int hour = util::hourOf(util::now());
    std::string greet = hour < 12 ? "Good morning" : (hour < 17 ? "Good afternoon" : "Good evening");
    ui::line(" " + c::bold() + greet + ", " + cu.name() + c::reset());
    ui::blank();
    accountCard(a);
    ui::blank();
    ui::title("Recent activity");
    auto hist = bank_.history(a.accNo());
    std::vector<Transaction> last(hist.end() - (long)std::min<size_t>(3, hist.size()), hist.end());
    std::reverse(last.begin(), last.end());
    std::vector<std::vector<std::string>> body;
    for (const auto& t : last) body.push_back({util::fmtShort(t.timestamp()), toString(t.type()), amountCell(t), util::money(t.balanceAfter())});
    if (body.empty()) ui::line(c::dim() + " No transactions yet." + c::reset());
    else ui::table({"Date", "Type", "Amount", "Balance"}, body, {Align::Left, Align::Left, Align::Right, Align::Right});
    size_t unread = bank_.unseenAlerts(a.accNo()).size();
    if (unread) ui::banner(Kind::Warning, std::to_string(unread) + " unread security alert(s). Open [8] Security & Alerts to review.");
    int openEmi = 0;
    for (const Loan* l : bank_.loansOf(a.accNo())) if (l->status() == LoanStatus::Active) ++openEmi;
    if (openEmi) ui::line(c::dim() + " " + std::to_string(openEmi) + " active loan(s). Next EMI is shown under [5] Loans & EMI." + c::reset());
    ui::blank();
}

void App::customerSession(User& user) {
    Customer& cu = static_cast<Customer&>(user);
    const std::string acc = cu.linkedAccount();
    for (;;) {
        ui::screen("Home > Customer > Dashboard");
        customerDashboard(cu);
        int k = choose({{1, "Transactions", "deposit, withdraw, transfer, balance"},
                        {2, "Smart statement", "filters, sorting, export, receipts"},
                        {3, "Spending insights", "category breakdown chart"},
                        {4, "Savings goals", "targets with progress bars"},
                        {5, "Loans & EMI", "apply, pay EMI, amortization"},
                        {6, "Beneficiaries", "saved payees"},
                        {7, "Interest & FD simulator", "what-if projections"},
                        {8, "Security & alerts", "fraud alerts, change PIN"},
                        {9, "Logout", ""}});
        switch (k) {
            case 1: transactionsMenu(acc); break;
            case 2: statementFlow(acc, "Home > Customer > Statement"); break;
            case 3: spendingReport(acc); break;
            case 4: goalsMenu(acc); break;
            case 5: loansMenu(acc); break;
            case 6: payeesMenu(acc); break;
            case 7: simulatorMenu(acc); break;
            case 8: securityMenu(cu); break;
            case 9: return;
        }
    }
}

void App::transactionsMenu(const std::string& acc) {
    for (;;) {
        ui::screen("Home > Customer > Transactions");
        accountCard(bank_.account(acc));
        ui::blank();
        int k = choose({{1, "Deposit", "add money to your account"},
                        {2, "Withdraw", "take cash out (with category)"},
                        {3, "Transfer", "to a saved payee or any account"},
                        {0, "Back", ""}});
        if (k == 0) return;
        try {
            if (k == 1) doDeposit(acc);
            else if (k == 2) doWithdraw(acc);
            else doTransfer(acc);
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

void App::doDeposit(const std::string& acc) {
    ui::screen("Home > Customer > Transactions > Deposit");
    double amt = ui::readAmount("Amount to deposit (" + std::string("\xE2\x82\xB9") + ")");
    const Transaction& tx = bank_.deposit(acc, amt);
    ui::banner(Kind::Success, "Deposited " + util::money(amt) + " successfully.");
    receipt(tx);
}

void App::doWithdraw(const std::string& acc) {
    ui::screen("Home > Customer > Transactions > Withdraw");
    const Account& a = bank_.account(acc);
    ui::line(c::dim() + " You can withdraw up to " + util::money(std::max(0.0, a.spendable())) + c::reset());
    double amt = ui::readAmount("Amount to withdraw (" + std::string("\xE2\x82\xB9") + ")");
    Category cat = pickCategory();
    double fee = bank_.fdWithdrawalFee(acc, amt);
    if (fee > 0) {
        ui::banner(Kind::Warning, "Early withdrawal from a fixed deposit costs a 1% penalty: " + util::money(fee) + ".");
        if (!ui::confirm("Continue and pay the penalty?")) { ui::banner(Kind::Info, "Withdrawal cancelled."); return; }
    } else if (!ui::confirm("Withdraw " + util::money(amt) + " (" + toString(cat) + ")?", true)) {
        ui::banner(Kind::Info, "Withdrawal cancelled.");
        return;
    }
    const Transaction& tx = bank_.withdraw(acc, amt, cat);
    ui::banner(Kind::Success, "Withdrawn " + util::money(amt) + " successfully.");
    receipt(tx);
}

void App::doTransfer(const std::string& acc) {
    ui::screen("Home > Customer > Transactions > Transfer");
    auto payees = bank_.payeesOf(acc);
    std::string target;
    if (!payees.empty()) {
        std::vector<std::vector<std::string>> body;
        for (size_t i = 0; i < payees.size(); ++i)
            body.push_back({std::to_string(i + 1), payees[i]->nickname(), payees[i]->payeeName(), util::maskAccount(payees[i]->payeeAcc())});
        ui::table({"#", "Nickname", "Name", "Account"}, body, {Align::Right, Align::Left, Align::Left, Align::Left});
        int pick = ui::readInt("Pick a saved payee number (0 = enter account number)", 0, (int)payees.size(), 0);
        if (pick > 0) target = payees[(size_t)pick - 1]->payeeAcc();
    }
    if (target.empty()) target = util::upper(ui::readLine("Destination account number"));
    const Account& dst = bank_.account(target);                    // throws AccountNotFound for bad numbers
    if (target == acc) throw InvalidAmountException("cannot transfer to the same account");
    ui::line(c::dim() + " Paying: " + dst.holder() + "  (" + util::maskAccount(dst.accNo()) + ")" + c::reset());
    double amt = ui::readAmount("Amount to transfer (" + std::string("\xE2\x82\xB9") + ")");
    Category cat = pickCategory();
    double fee = bank_.fdWithdrawalFee(acc, amt);
    if (fee > 0) ui::banner(Kind::Warning, "Early FD withdrawal penalty of " + util::money(fee) + " will apply.");
    ui::box("Confirm transfer", {"To      " + dst.holder() + " (" + util::maskAccount(dst.accNo()) + ")",
                                 "Amount  " + c::bold() + util::money(amt) + c::reset(), "Purpose " + toString(cat)});
    if (!ui::confirm("Confirm this transfer?")) { ui::banner(Kind::Info, "Transfer cancelled."); return; }
    const Transaction& tx = bank_.transfer(acc, target, amt, cat);
    ui::banner(Kind::Success, "Transferred " + util::money(amt) + " to " + dst.holder() + ".");
    receipt(tx);
}

void App::spendingReport(const std::string& acc) {
    ui::screen("Home > Customer > Spending insights");
    std::string month = ui::readLine("Month (YYYY-MM) " + c::dim() + "or Enter for all time" + c::reset());
    auto slices = SpendingAnalyzer::analyze(bank_.history(acc), month);
    ui::blank();
    if (slices.empty()) { ui::banner(Kind::Info, "No categorised spending found for that period."); ui::pause(); return; }
    double total = 0;
    std::vector<std::string> labels, text;
    std::vector<double> vals;
    for (const auto& s : slices) {
        total += s.total;
        labels.push_back(toString(s.category));
        vals.push_back(s.total);
        char pct[16]; std::snprintf(pct, sizeof pct, "%4.1f%%", s.percent);
        text.push_back(util::money(s.total) + "  " + pct);
    }
    ui::title("Spending by category  " + std::string(month.empty() ? "(all time)" : "(" + month + ")"));
    ui::barChart(labels, vals, text, 26, 0);
    ui::blank();
    ui::line(" Total spent: " + c::bold() + c::red() + util::money(total) + c::reset() + "   Top category: " + c::yellow() + labels[0] + c::reset());
    ui::pause();
}

void App::goalsMenu(const std::string& acc) {
    for (;;) {
        ui::screen("Home > Customer > Savings goals");
        auto goals = bank_.goalsOf(acc);
        if (goals.empty()) ui::banner(Kind::Info, "You have no savings goals yet. Create one!");
        else {
            std::vector<std::vector<std::string>> body;
            for (const SavingsGoal* g : goals) {
                std::string st = g->status(util::now());
                std::string col = st == "Achieved" ? c::green() : (st == "Overdue" ? c::red() : c::cyan());
                body.push_back({g->id(), g->name(), ui::progressBar(g->fraction(), 8),
                                util::moneyShort(g->saved()) + " / " + util::moneyShort(g->target()), ui::paint(col, st)});
            }
            ui::table({"ID", "Goal", "Progress", "Saved / Target", "Status"}, body);
            for (const SavingsGoal* g : goals)
                if (g->status(util::now()) == "Active")
                    ui::line(c::dim() + " " + g->id() + " " + g->name() + ": due " + util::fmtDate(g->deadline()) + ", save " +
                             util::money(g->requiredMonthly(util::now())) + " per month" + c::reset());
        }
        ui::blank();
        int k = choose({{1, "Create a goal", "name, target and deadline"}, {2, "Add money to a goal", "moves money from your balance"}, {0, "Back", ""}});
        if (k == 0) return;
        try {
            if (k == 1) {
                std::string name = ui::readLine("Goal name (e.g. Laptop)");
                double target = ui::readAmount("Target amount (" + std::string("\xE2\x82\xB9") + ")");
                std::optional<time_t> dl;
                for (;;) { askDate("Deadline", dl); if (dl) break; ui::banner(Kind::Error, "A deadline is required."); }
                const SavingsGoal& g = bank_.createGoal(acc, name, target, *dl);
                ui::banner(Kind::Success, "Goal " + g.id() + " created. Save " + util::money(g.requiredMonthly(util::now())) + " per month to reach it.");
            } else {
                if (goals.empty()) throw BankException("Create a goal first.");
                std::string id = util::upper(ui::readLine("Goal ID (e.g. G001)"));
                bool mine = std::any_of(goals.begin(), goals.end(), [&](const SavingsGoal* g) { return g->id() == id; });
                if (!mine) throw BankException("That goal does not belong to your account.");
                double amt = ui::readAmount("Amount to add (" + std::string("\xE2\x82\xB9") + ")");
                const Transaction& tx = bank_.contributeToGoal(id, amt);
                ui::banner(Kind::Success, "Added " + util::money(amt) + " to your goal.");
                receipt(tx);
            }
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

void App::loansMenu(const std::string& acc) {
    for (;;) {
        ui::screen("Home > Customer > Loans & EMI");
        auto loans = bank_.loansOf(acc);
        if (loans.empty()) ui::banner(Kind::Info, "You have no loans.");
        else {
            loanTable(loans);
            for (const Loan* l : loans)
                if (l->status() == LoanStatus::Active)
                    ui::line(c::dim() + " " + l->id() + ": next EMI " + util::money(l->nextInstallment()) + "  (" +
                             std::to_string(l->remainingInstallments()) + " left)" + c::reset());
        }
        ui::blank();
        int k = choose({{1, "Apply for a loan", "Home, Education or Personal"},
                        {2, "EMI calculator", "estimate without applying"},
                        {3, "Amortization table", "month-by-month schedule"},
                        {4, "Pay next EMI", "from your account balance"},
                        {5, "Prepay and close", "pay the outstanding principal"},
                        {0, "Back", ""}});
        if (k == 0) return;
        try {
            if (k == 1 || k == 2) {
                ui::line(c::cyan() + " [1]" + c::reset() + " Home       8.5% p.a.   up to " + util::moneyShort(5000000) + "   max 240 months");
                ui::line(c::cyan() + " [2]" + c::reset() + " Education  9.5% p.a.   up to " + util::moneyShort(1500000) + "   max 120 months");
                ui::line(c::cyan() + " [3]" + c::reset() + " Personal  12.5% p.a.   up to " + util::moneyShort(500000) + "   max 60 months");
                static const char* codes[] = {"HOME", "EDU", "PER"};
                std::string code = codes[ui::readInt("Choose loan type", 1, 3) - 1];
                double amt = ui::readAmount("Loan amount (" + std::string("\xE2\x82\xB9") + ")");
                int months = ui::readInt("Tenure in months", 1, 600);
                if (k == 2) {
                    auto probe = Loan::create(code, "-", acc, amt, months, LoanStatus::Pending, 0, util::now());
                    emiPreview(*probe);
                    amortizationTable(*probe);
                } else {
                    auto probe = Loan::create(code, "-", acc, amt, months, LoanStatus::Pending, 0, util::now());
                    if (amt >= 10000 && amt <= probe->maxPrincipal() && months >= 6 && months <= probe->maxMonths()) emiPreview(*probe);
                    if (ui::confirm("Submit this loan application?", true)) {
                        const Loan& l = bank_.applyLoan(acc, code, amt, months);
                        ui::banner(Kind::Success, "Application " + l.id() + " submitted. An admin will review it shortly.");
                    }
                }
            } else {
                std::string id = util::upper(ui::readLine("Loan ID (e.g. L001)"));
                const Loan* l = bank_.findLoan(id);
                if (!l || l->accNo() != acc) throw LoanException("That loan does not belong to your account.");
                if (k == 3) { emiPreview(*l); amortizationTable(*l); }
                else if (k == 4) {
                    if (!ui::confirm("Pay EMI of " + util::money(l->nextInstallment()) + " now?", true)) { ui::banner(Kind::Info, "Cancelled."); }
                    else { const Transaction& tx = bank_.payEMI(id); ui::banner(Kind::Success, "EMI paid."); receipt(tx); }
                } else {
                    if (!ui::confirm("Prepay " + util::money(l->outstanding()) + " and close loan " + id + "?")) ui::banner(Kind::Info, "Cancelled.");
                    else { const Transaction& tx = bank_.prepayLoan(id); ui::banner(Kind::Success, "Loan closed."); receipt(tx); }
                }
            }
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

void App::payeesMenu(const std::string& acc) {
    for (;;) {
        ui::screen("Home > Customer > Beneficiaries");
        auto payees = bank_.payeesOf(acc);
        if (payees.empty()) ui::banner(Kind::Info, "No saved beneficiaries.");
        else {
            std::vector<std::vector<std::string>> body;
            for (const Beneficiary* b : payees)
                body.push_back({b->nickname(), b->payeeName(), util::maskAccount(b->payeeAcc()), util::fmtDate(b->addedOn())});
            ui::table({"Nickname", "Name", "Account", "Added"}, body);
        }
        ui::blank();
        int k = choose({{1, "Add beneficiary", "save a payee with a nickname"}, {2, "Remove beneficiary", ""}, {0, "Back", ""}});
        if (k == 0) return;
        try {
            if (k == 1) {
                std::string nick = ui::readLine("Nickname (max 15 chars)");
                std::string no = util::upper(ui::readLine("Payee account number"));
                const Account& pa = bank_.account(no);
                ui::line(c::dim() + " Account holder: " + pa.holder() + c::reset());
                if (ui::confirm("Save '" + nick + "' as a beneficiary?", true)) {
                    bank_.addPayee(acc, nick, no);
                    ui::banner(Kind::Success, "Beneficiary saved. Transfers to new payees are monitored for 24 hours.");
                }
            } else {
                std::string nick = ui::readLine("Nickname to remove");
                bank_.removePayee(acc, nick);
                ui::banner(Kind::Success, "Beneficiary removed.");
            }
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

void App::simulatorMenu(const std::string& acc) {
    ui::screen("Home > Customer > Interest & FD simulator");
    const Account& mine = bank_.account(acc);
    double def = std::max(1000.0, std::floor(mine.balance()));
    ui::line(c::dim() + " Compare what the same amount would earn in each account type." + c::reset());
    double principal = ui::readOptionalAmount("Amount to invest", "(Enter for " + util::money(def) + ")").value_or(def);
    if (principal <= 0) principal = def;
    int years = ui::readInt("Years", 1, 20, 5);
    SavingsAccount sav("-", "-", principal, 0, 4.0);
    StudentSavingsAccount stu("-", "-", principal, 0, 4.5);
    FixedDepositAccount fd("-", "-", principal, 0, 7.0, 12);
    CurrentAccount cur("-", "-", principal, 0);
    auto rs = Bank::simulate(sav, principal, years), rt = Bank::simulate(stu, principal, years);
    auto rf = Bank::simulate(fd, principal, years);
    std::vector<std::vector<std::string>> body;
    for (int y = 0; y < years; ++y)
        body.push_back({std::to_string(y + 1), util::money(rs[(size_t)y].simple), util::money(rt[(size_t)y].simple),
                        ui::paint(c::green(), util::money(rf[(size_t)y].compound)), util::money(principal)});
    ui::blank();
    ui::table({"Year", "Savings 4%", "Student 4.5%", "FD 7% comp.", "Current 0%"}, body,
              {Align::Right, Align::Right, Align::Right, Align::Right, Align::Right}, 22);
    ui::line(c::dim() + " Savings/Student: simple interest. FD: compounded yearly. Current: none." + c::reset());
    ui::blank();
    ui::box("Maturity after " + std::to_string(years) + " year(s)", {
        "Savings  " + util::money(principal + sav.calculateInterest(years)) + "   (interest " + util::money(sav.calculateInterest(years)) + ")",
        "FD       " + c::bold() + c::green() + util::money(principal + fd.calculateInterest(years)) + c::reset() + "   (interest " + util::money(fd.calculateInterest(years)) + ")",
        "Current  " + util::money(principal + cur.calculateInterest(years)) + "   (no interest)",
        c::dim() + "Your own account (" + mine.typeName() + ") would earn " + util::money(mine.calculateInterest(years)) + " on " +
            util::money(mine.balance()) + c::reset()});
    ui::pause();
}

void App::securityMenu(Customer& cu) {
    for (;;) {
        ui::screen("Home > Customer > Security & alerts");
        auto alerts = bank_.alertsOf(cu.linkedAccount());
        ui::title("Fraud alerts");
        if (alerts.empty()) ui::banner(Kind::Success, "No alerts. Your account looks healthy.");
        else {
            std::vector<std::vector<std::string>> body;
            for (const Alert* a : alerts)
                body.push_back({a->id(), util::fmtShort(a->timestamp()), a->seen() ? "Read" : ui::paint(c::yellow(), "NEW"), a->message()});
            ui::table({"ID", "When", "Status", "What happened"}, body, {Align::Left, Align::Left, Align::Left, Align::Left}, 44);
            bank_.markAlertsSeen(cu.linkedAccount());
            ui::line(c::dim() + " If you do not recognise a transaction, contact the bank immediately." + c::reset());
        }
        ui::blank();
        int k = choose({{1, "Change PIN", "4 digits, old PIN required"}, {0, "Back", ""}});
        if (k == 0) return;
        try {
            std::string oldPin = ui::readPin("Current PIN");
            std::string n1 = ui::readPin("New PIN (4 digits)");
            std::string n2 = ui::readPin("Repeat new PIN");
            if (n1 != n2) throw BankException("The two new PINs do not match.");
            bank_.changePin(cu, oldPin, n1);
            ui::banner(Kind::Success, "PIN changed successfully.");
        } catch (const AuthenticationException&) { ui::banner(Kind::Error, "Current PIN is incorrect.");
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}
