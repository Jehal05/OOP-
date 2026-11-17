#include "Fraud.h"
#include "Utils.h"
#include <cstdlib>

std::string Alert::toCsv() const {
    return id + "," + accNo + "," + rule + "," + util::clean(message) + "," + std::to_string((long long)time) + "," +
           (reviewed ? "1" : "0") + "," + (seen ? "1" : "0");
}

Alert Alert::fromCsv(const std::string& line) {
    auto f = util::split(line);
    while (f.size() < 7) f.push_back("");
    return Alert{f[0], f[1], f[2], f[3], (time_t)std::atoll(f[4].c_str()), f[5] == "1", f[6] == "1"};
}

// debit of Rs 50,000 or more
bool LargeAmountRule::check(const Transaction& tx, const std::vector<Transaction>&, std::string& message) const {
    if (tx.amount < 50000) return false;
    message = "Large debit of " + util::money(tx.amount) + " (" + tx.type + ")";
    return true;
}

// 5 or more debits within 10 minutes
bool RapidTransactionsRule::check(const Transaction& tx, const std::vector<Transaction>& history, std::string& message) const {
    int count = 1;                                       // the new one
    for (const auto& h : history)
        if (!h.credit && h.ref != tx.ref && tx.time - h.time <= 600 && tx.time >= h.time) ++count;
    if (count < 5) return false;
    message = std::to_string(count) + " debits within 10 minutes";
    return true;
}

// debit between 1 AM and 4 AM
bool OddHourRule::check(const Transaction& tx, const std::vector<Transaction>&, std::string& message) const {
    int hour = util::hourOf(tx.time);
    if (hour < 1 || hour >= 4) return false;
    message = "Debit of " + util::money(tx.amount) + " at an unusual hour (" + util::formatTime(tx.time) + ")";
    return true;
}
