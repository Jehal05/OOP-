// One line of the account history. Transactions are only ever added, never changed.
#pragma once
#include <ctime>
#include <string>
#include "Utils.h"

struct Transaction {
    std::string ref;          // e.g. TXN-20261116-000012
    std::string accNo;
    std::string type;         // Deposit, Withdraw, Transfer Out, Transfer In, Interest, Loan Credit, EMI, Fee
    bool credit;              // true = money in, false = money out
    double amount;
    double balanceAfter;
    time_t time;
    std::string note;

    std::string toCsv() const {
        char buf[64];
        std::string s = ref + "," + accNo + "," + type + "," + (credit ? "1" : "0") + ",";
        std::snprintf(buf, sizeof buf, "%.2f,%.2f,", amount, balanceAfter);
        return s + buf + std::to_string((long long)time) + "," + util::clean(note);
    }

    static Transaction fromCsv(const std::string& line) {
        auto f = util::split(line);
        while (f.size() < 8) f.push_back("");
        return Transaction{f[0], f[1], f[2], f[3] == "1", std::atof(f[4].c_str()), std::atof(f[5].c_str()),
                           (time_t)std::atoll(f[6].c_str()), f[7]};
    }
};
