// AdminApp.cpp - the staff side of the application.
#include "App.h"
#include "Utils.h"
#include <algorithm>
#include <iostream>

using ui::Kind;
using ui::Align;

static void accountTable(const std::vector<Account*>& accs) {
    std::vector<std::vector<std::string>> body;
    for (const Account* a : accs)
        body.push_back({a->accNo(), a->typeName(), a->holder(), util::money(a->balance()),
                        a->active() ? "Active" : "Closed"});
    if (body.empty()) { ui::banner(Kind::Info, "No accounts found."); return; }
    ui::table({"Account", "Type", "Holder", "Balance", "Status"}, body,
              {Align::Left, Align::Left, Align::Left, Align::Right, Align::Left});
}

void App::adminSession(User& admin) {
    const std::string id = admin.id();
    for (;;) {
        ui::screen("Home > Admin > Dashboard");
        auto pending = bank_.pendingLoans();
        auto alerts = bank_.alertsAwaitingReview();
        auto locked = bank_.lockedUsers();
        ui::line(" Welcome, " + admin.name());
        ui::blank();
        ui::box("Bank overview", {
            "Accounts  " + std::to_string(bank_.allAccounts().size()) + "     Total balance  " + util::money(bank_.totalBalance()),
            "Pending loans  " + (pending.empty() ? "0" : std::to_string(pending.size())) +
                "     Alerts to review  " + (alerts.empty() ? "0" : std::to_string(alerts.size())) +
                "     Locked users  " + (locked.empty() ? "0" : std::to_string(locked.size()))});
        ui::blank();
        int k = choose({{1, "Accounts", "create, search, update, close, list"},
                        {2, "Transaction history", "statement of any account"},
                        {3, "Apply monthly interest", "credit savings interest"},
                        {4, "Loan approvals", "approve or reject applications"},
                        {5, "Fraud alerts", "review flagged transactions"},
                        {6, "Reports", "deposits, richest, dormant, totals"},
                        {7, "Security & audit log", "unlock users, view log"},
                        {9, "Logout", ""}});
        try {
            switch (k) {
                case 1: adminAccounts(id); break;
                case 2: {
                    ui::screen("Home > Admin > Transaction history");
                    std::string no = util::upper(ui::readLine("Account number"));
                    bank_.account(no);
                    bank_.logAudit(id, "VIEW_HISTORY", no);
                    statementFlow(no, "Home > Admin > History > " + no);
                    break;
                }
                case 3: adminInterest(id); break;
                case 4: adminLoans(id); break;
                case 5: adminAlerts(id); break;
                case 6: adminReports(); break;
                case 7: adminSecurity(id); break;
                case 9: return;
            }
        } catch (const BankException& e) { showError(e); ui::pause(); }
    }
}

void App::adminOpenAccount(const std::string& adminId) {
    ui::screen("Home > Admin > Accounts > Open account");
    printChoices("Account types:", {"Savings", "Student", "Current", "Fixed Dep."}, 4);
    int t = ui::readInt("Choose account type", 1, 4);
    static const char* codes[] = {"SAV", "STU", "CUR", "FD"};
    std::string code = codes[t - 1];
    std::string name = ui::readLine("Holder full name");
    double minOpen = code == "SAV" ? 1000 : (code == "CUR" ? 5000 : (code == "FD" ? 10000 : 0));
    ui::line(" Minimum opening deposit: " + util::money(minOpen));
    double initial = minOpen == 0 ? ui::readOptionalAmount("Opening deposit").value_or(0) : ui::readAmount("Opening deposit (Rs)");
    double p1 = code == "STU" ? 4.5 : (code == "FD" ? 7.0 : (code == "CUR" ? 25000.0 : 4.0)), p2 = code == "SAV" ? 1000.0 : 0.0;
    if (code == "CUR") p1 = ui::readOptionalAmount("Overdraft limit (default " + util::money(25000) + ")").value_or(25000.0);
    if (code == "FD") p2 = ui::readInt("Lock-in period in months", 6, 120, 12);
    std::string pin = ui::readPin("Set customer PIN (4 digits)");
    if (pin != ui::readPin("Repeat PIN")) throw BankException("The two PINs do not match.");
    std::string no = bank_.openAccount(adminId, code, name, initial, pin, p1, p2);
    ui::banner(Kind::Success, "Account opened.");
    accountCard(bank_.account(no));
    ui::line(" Customer login:  account " + no + " with the PIN you just set.");
}

void App::adminAccounts(const std::string& adminId) {
    for (;;) {
        ui::screen("Home > Admin > Accounts");
        int k = choose({{1, "Open account", "new customer and login"},
                        {2, "Search accounts", "by number or name"},
                        {3, "Update holder name", ""},
                        {4, "Close account", "pays out remaining balance"},
                        {5, "List all accounts", ""},
                        {0, "Back", ""}});
        if (k == 0) return;
        try {
            if (k == 1) adminOpenAccount(adminId);
            else if (k == 2) {
                std::string q = ui::readLine("Search text");
                accountTable(bank_.searchAccounts(q));
            } else if (k == 3) {
                std::string no = util::upper(ui::readLine("Account number"));
                const Account& a = bank_.account(no);
                ui::line(" Current name: " + a.holder());
                bank_.updateHolder(adminId, no, ui::readLine("New holder name"));
                ui::banner(Kind::Success, "Account updated.");
            } else if (k == 4) {
                std::string no = util::upper(ui::readLine("Account number"));
                accountCard(bank_.account(no));
                if (ui::confirm("Close this account permanently and pay out the balance?")) {
                    double payout = bank_.closeAccount(adminId, no);
                    ui::banner(Kind::Success, "Account closed. Payout: " + util::money(payout) + ".");
                } else ui::banner(Kind::Info, "Cancelled.");
            } else accountTable(bank_.allAccounts());
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

void App::adminInterest(const std::string& adminId) {
    ui::screen("Home > Admin > Apply monthly interest");
    ui::line(" Credits one month of interest (rate / 12) to every active savings account.");
    ui::line(" It is safe to run again: each account is credited at most once per month.");
    if (!ui::confirm("Apply interest now?")) { ui::banner(Kind::Info, "Cancelled."); ui::pause(); return; }
    auto res = bank_.applyMonthlyInterest(adminId);
    std::vector<std::vector<std::string>> body;
    for (const auto& r : res)
        body.push_back({r.accNo, r.holder, r.amount > 0 ? "+" + util::money(r.amount) : "-", r.status});
    ui::table({"Account", "Holder", "Interest", "Status"}, body, {Align::Left, Align::Left, Align::Right, Align::Left});
    ui::pause();
}

void App::adminLoans(const std::string& adminId) {
    for (;;) {
        ui::screen("Home > Admin > Loan approvals");
        auto pending = bank_.pendingLoans();
        if (pending.empty()) { ui::banner(Kind::Success, "No pending loan applications."); ui::pause(); return; }
        loanTable(pending, true);
        ui::blank();
        int k = choose({{1, "Approve a loan", "credits the amount to the account"}, {2, "Reject a loan", "with a reason"},
                        {3, "View EMI details", ""}, {0, "Back", ""}});
        if (k == 0) return;
        try {
            std::string id = util::upper(ui::readLine("Loan ID"));
            const Loan* l = bank_.findLoan(id);
            if (!l) throw LoanException("Loan not found.");
            if (k == 3) { emiPreview(*l); amortizationTable(*l); }
            else if (k == 1) {
                emiPreview(*l);
                if (ui::confirm("Approve " + id + " and credit " + util::money(l->principal()) + " to " + l->accNo() + "?")) {
                    bank_.approveLoan(adminId, id);
                    ui::banner(Kind::Success, "Loan approved and disbursed.");
                } else ui::banner(Kind::Info, "Cancelled.");
            } else {
                std::string reason = ui::readLine("Reason for rejection");
                bank_.rejectLoan(adminId, id, reason);
                ui::banner(Kind::Success, "Loan rejected.");
            }
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

void App::adminAlerts(const std::string& adminId) {
    for (;;) {
        ui::screen("Home > Admin > Fraud alerts");
        auto alerts = bank_.alertsAwaitingReview();
        ui::line(" " + std::to_string(bank_.fraudRuleCount()) + " fraud rules are active.");
        ui::blank();
        if (alerts.empty()) { ui::banner(Kind::Success, "No alerts awaiting review."); ui::pause(); return; }
        std::vector<std::vector<std::string>> body;
        for (const Alert* a : alerts)
            body.push_back({a->id(), a->accNo(), util::fmtShort(a->timestamp()), a->rule()});
        ui::table({"ID", "Account", "When", "Rule"}, body);
        for (const Alert* a : alerts) ui::line(" " + a->id() + "  " + ui::truncate(a->message(), 70));
        ui::blank();
        std::string id = util::upper(ui::readLine("Alert ID to mark as reviewed (Enter to go back)"));
        if (id.empty()) return;
        try {
            bank_.markAlertReviewed(adminId, id);
            ui::banner(Kind::Success, "Alert " + id + " marked as reviewed.");
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}

void App::adminReports() {
    for (;;) {
        ui::screen("Home > Admin > Reports");
        int k = choose({{1, "Overview", "balances and deposits"},
                        {2, "Richest accounts", "top balances with chart"},
                        {3, "Dormant accounts", "no activity for N days"},
                        {4, "Daily totals", "credits and debits per day"},
                        {5, "Monthly totals", "credits and debits per month"},
                        {0, "Back", ""}});
        if (k == 0) return;
        ui::blank();
        if (k == 1) {
            std::map<std::string, int> byType;
            for (const Account* a : bank_.allAccounts()) if (a->active()) ++byType[a->typeName()];
            std::vector<std::string> lines = {"Total deposits ever made   " + util::money(bank_.totalDeposits()),
                                              "Total balance held         " + util::money(bank_.totalBalance()), ""};
            for (const auto& [t, n] : byType) lines.push_back(ui::padRight(t, 18) + std::to_string(n) + " account(s)");
            ui::box("Bank overview", lines);
        } else if (k == 2) {
            auto top = bank_.richest(5);
            std::vector<std::string> labels, text;
            std::vector<double> vals;
            for (const Account* a : top) { labels.push_back(a->accNo() + " " + a->holder()); vals.push_back(std::max(0.0, a->balance())); text.push_back(util::money(a->balance())); }
            ui::title("Top accounts by balance");
            ui::list(labels, text);
        } else if (k == 3) {
            int days = ui::readInt("Inactive for at least how many days", 1, 3650, 30);
            auto d = bank_.dormant(days);
            if (d.empty()) ui::banner(Kind::Success, "No dormant accounts.");
            else {
                std::vector<std::vector<std::string>> body;
                for (const auto& [a, last] : d) body.push_back({a->accNo(), a->holder(), util::fmtDate(last), util::money(a->balance())});
                ui::table({"Account", "Holder", "Last activity", "Balance"}, body, {Align::Left, Align::Left, Align::Left, Align::Right});
            }
        } else {
            auto totals = bank_.periodTotals(k == 5);
            std::vector<std::vector<std::string>> body;
            size_t shown = 0, skip = totals.size() > 12 ? totals.size() - 12 : 0;
            for (const auto& [key, v] : totals) {
                if (shown++ < skip) continue;
                double net = v.first - v.second;
                body.push_back({key, util::money(v.first), util::money(v.second),
                                (net >= 0 ? "+" : "") + util::money(net)});
            }
            ui::table({k == 5 ? "Month" : "Date", "Credits", "Debits", "Net"}, body, {Align::Left, Align::Right, Align::Right, Align::Right});
        }
        ui::pause();
    }
}

void App::adminSecurity(const std::string& adminId) {
    for (;;) {
        ui::screen("Home > Admin > Security & audit log");
        auto locked = bank_.lockedUsers();
        ui::title("Locked users");
        if (locked.empty()) ui::banner(Kind::Success, "No locked users.");
        else {
            std::vector<std::vector<std::string>> body;
            for (const User* u : locked) body.push_back({u->id(), u->name(), u->role(), "LOCKED"});
            ui::table({"User", "Name", "Role", "Status"}, body);
        }
        ui::blank();
        int k = choose({{1, "Unlock a user", "reset failed attempts"}, {2, "View audit log", "latest entries"},
                        {3, "Search audit log", "by keyword"}, {0, "Back", ""}});
        if (k == 0) return;
        try {
            if (k == 1) {
                std::string id = util::upper(ui::readLine("User / account to unlock"));
                bank_.unlockUser(adminId, id);
                ui::banner(Kind::Success, id + " has been unlocked.");
            } else {
                auto lines = k == 2 ? bank_.audit().readAll() : bank_.audit().search(ui::readLine("Keyword"));
                ui::blank();
                if (lines.empty()) ui::banner(Kind::Info, "No matching entries.");
                else {
                    size_t from = lines.size() > 18 ? lines.size() - 18 : 0;
                    ui::title("Audit log  (" + std::to_string(lines.size() - from) + " of " + std::to_string(lines.size()) + " entries, newest last)");
                    for (size_t i = from; i < lines.size(); ++i) ui::line(" " + ui::truncate(lines[i], 76));
                }
            }
        } catch (const BankException& e) { showError(e); }
        ui::pause();
    }
}
