// Exceptions used by the bank. They all derive from BankException, so main() can catch one type.
#pragma once
#include <stdexcept>
#include <string>

class BankException : public std::runtime_error {
public:
    explicit BankException(const std::string& msg) : std::runtime_error(msg) {}
};

class InsufficientFundsException : public BankException {
public:
    InsufficientFundsException() : BankException("Insufficient balance for this transaction.") {}
};

class AccountLockedException : public BankException {
public:
    AccountLockedException() : BankException("This account is locked. Please ask the bank admin to unlock it.") {}
};

class NotFoundException : public BankException {
public:
    explicit NotFoundException(const std::string& what) : BankException(what + " not found.") {}
};

class InvalidInputException : public BankException {
public:
    explicit InvalidInputException(const std::string& msg) : BankException(msg) {}
};
