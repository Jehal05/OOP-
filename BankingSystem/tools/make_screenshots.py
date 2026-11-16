#!/usr/bin/env python3
"""Regenerate docs/img/screens/*.png by running the scripted scenarios against the real binary."""
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run_scenario, render_ansi

ANSI = re.compile(r"\x1b\[[0-9;]*m")
OUT = os.path.join(run_scenario.ROOT, "docs", "img", "screens")
SC = lambda n: os.path.join(run_scenario.ROOT, "scenarios", n + ".txt")

SHOTS = [  # (file, scenario, title, predicate on the plain-text screen)
    ("01_home",            "customer_tour", "Home screen",                 lambda s: "Demo logins" in s),
    ("02_dashboard",       "customer_tour", "Customer dashboard",          lambda s: "Recent activity" in s),
    ("03_deposit_receipt", "customer_tour", "Deposit and receipt",         lambda s: "TRANSACTION RECEIPT" in s and "Cash deposit" in s),
    ("04_transfer",        "customer_tour", "Transfer to a saved payee",   lambda s: "Confirm transfer" in s),
    ("05_smart_statement", "customer_tour", "Smart statement (filtered, sorted)", lambda s: "Filters: type = Withdraw" in s),
    ("06_monthly_summary", "customer_tour", "Monthly summary",             lambda s: "Monthly summary" in s and "Credits" in s),
    ("07_statement_export","customer_tour", "Statement export",            lambda s: "Statement saved to" in s),
    ("08_spending_chart",  "customer_tour", "Spending categories chart",   lambda s: "Spending by category" in s),
    ("09_savings_goals",   "customer_tour", "Savings goals",               lambda s: "G003" in s and "Progress" in s and "RECEIPT" not in s and "Add money" in s),
    ("10_emi_calculator",  "customer_tour", "EMI calculator",              lambda s: "EMI Estimate" in s and "Home Loan" in s),
    ("11_amortization",    "customer_tour", "Amortization table",          lambda s: "Loan ID (e.g. L001) L001" in s and "Principal" in s),
    ("12_simulator",       "customer_tour", "Interest and FD simulator",   lambda s: "Maturity after" in s),
    ("13_fraud_alerts",    "lockout_and_fraud", "Customer fraud alerts",   lambda s: "A002" in s and "NEW" in s),
    ("14_lockout",         "lockout_and_fraud", "PIN lockout",             lambda s: "locked after too many" in s),
    ("15_fd_penalty",      "lockout_and_fraud", "FD early-withdrawal penalty", lambda s: "1% penalty" in s),
    ("16_admin_dashboard", "admin_tour", "Admin dashboard",                lambda s: "Bank overview" in s and "Welcome, Bank Administrator" in s),
    ("17_admin_open",      "admin_tour", "Open account",                   lambda s: "Account opened" in s),
    ("18_admin_loans",     "admin_tour", "Loan approvals",                 lambda s: "Loan approvals" in s and "Pending" in s and "Approve a loan" in s),
    ("19_admin_fraud",     "admin_tour", "Fraud alert review",             lambda s: "Alert ID to mark" in s),
    ("20_admin_richest",   "admin_tour", "Report: richest accounts",       lambda s: "Top accounts by balance" in s),
    ("21_admin_audit",     "admin_tour", "Audit log",                      lambda s: "Audit log  (" in s),
]

def main():
    os.makedirs(OUT, exist_ok=True)
    cache, missing = {}, 0
    for name, scn, title, pred in SHOTS:
        if scn not in cache:
            cache[scn] = run_scenario.run(SC(scn), color=True)[0]
        match = next((s for s in cache[scn] if pred(ANSI.sub("", s))), None)
        if match is None:
            print("MISSING", name); missing += 1; continue
        size = render_ansi.render(match, os.path.join(OUT, name + ".png"), title="nova-bank  -  " + title)
        print(f"{name}.png {size}")
    sys.exit(1 if missing else 0)

if __name__ == "__main__":
    main()
