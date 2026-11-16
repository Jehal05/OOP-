// Exceptions.h - custom exception hierarchy for the banking system.
#pragma once
#include <exception>
#include <string>

// Base class for every domain error. Catch this to handle any banking failure.
class BankException : public std::exception {
protected:
    std::string msg_;
public:
    explicit BankException(std::string m) : msg_(std::move(m)) {}
    const char* what() const noexcept override { return msg_.c_str(); }
};

class InsufficientFundsException : public BankException {
    double available_, requested_;
public:
    InsufficientFundsException(double available, double requested, const std::string& detail)
        : BankException("Insufficient funds. " + detail), available_(available), requested_(requested) {}
    double available() const { return available_; }
    double requested() const { return requested_; }
};

class InvalidAmountException : public BankException {
public:
    explicit InvalidAmountException(const std::string& why) : BankException("Invalid amount: " + why) {}
};

class AccountNotFoundException : public BankException {
public:
    explicit AccountNotFoundException(const std::string& id) : BankException("Account not found: " + id) {}
};

class AuthenticationException : public BankException {
    int attemptsLeft_;
public:
    explicit AuthenticationException(int attemptsLeft = -1)
        : BankException("Invalid credentials."), attemptsLeft_(attemptsLeft) {}
    int attemptsLeft() const { return attemptsLeft_; }
};

class AccountLockedException : public BankException {
public:
    AccountLockedException() : BankException("This account is locked after too many wrong PIN attempts. Please contact the bank (admin) to unlock it.") {}
};

class LoanException : public BankException {
public:
    explicit LoanException(const std::string& m) : BankException(m) {}
};

class FileIOException : public BankException {
public:
    explicit FileIOException(const std::string& m) : BankException("File error: " + m) {}
};

// Thrown when stdin is closed (EOF) so the app can save and exit cleanly.
struct InputClosed {};
