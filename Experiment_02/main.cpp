// Experiment 2: Class hierarchy (Account -> SavingsAccount) with access specifiers and member functions
#include <iostream>
#include <string>
using namespace std;

class Account {
private:
    double balance;          // only this class can touch it
protected:
    string name;             // this class and derived classes
public:
    Account(string n, double b) {
        name = n;
        balance = b;
    }
    void deposit(double amount) { balance += amount; }
    void withdraw(double amount) {
        if (amount > balance) cout << "Not enough balance" << endl;
        else balance -= amount;
    }
    double getBalance() { return balance; }
    void show() { cout << name << " has Rs " << balance << endl; }
};

class SavingsAccount : public Account {
private:
    double rate;
public:
    SavingsAccount(string n, double b, double r) : Account(n, b) { rate = r; }
    void addInterest() { deposit(getBalance() * rate / 100); }
    void showName() { cout << "Account holder: " << name << endl; }
};

int main() {
    Account a("Asha", 5000);
    a.deposit(1000);
    a.withdraw(500);
    a.show();

    SavingsAccount s("Ravi", 10000, 4);
    s.addInterest();
    s.showName();
    s.show();
    return 0;
}
