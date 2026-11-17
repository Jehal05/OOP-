// Experiment 9: Custom exceptions for banking transactions
#include <iostream>
#include <exception>
using namespace std;

class InsufficientFunds : public exception {
public:
    const char* what() const noexcept { return "Insufficient funds"; }
};

class InvalidAmount : public exception {
public:
    const char* what() const noexcept { return "Invalid amount"; }
};

class Account {
    double balance;
public:
    Account(double b) { balance = b; }
    void withdraw(double amount) {
        if (amount <= 0) throw InvalidAmount();
        if (amount > balance) throw InsufficientFunds();
        balance -= amount;
        cout << "Withdrawn " << amount << ", balance = " << balance << endl;
    }
};

int main() {
    Account acc(1000);
    try {
        acc.withdraw(300);
        acc.withdraw(-50);          // throws InvalidAmount
    } catch (exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    try {
        acc.withdraw(5000);         // throws InsufficientFunds
    } catch (exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    return 0;
}
