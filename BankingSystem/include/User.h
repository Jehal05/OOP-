// User.h - login identities.  User (abstract) -> Customer, Admin
#pragma once
#include <string>
#include <memory>

class User {
    std::string id_, name_, salt_, hash_;
    int failed_;
    bool locked_;
public:
    static constexpr int MAX_ATTEMPTS = 3;

    User(std::string id, std::string name, const std::string& pin);                         // new user, hashes PIN
    User(std::string id, std::string name, std::string salt, std::string hash, int failed, bool locked);
    virtual ~User() = default;

    virtual std::string role() const = 0;                       // "CUSTOMER" / "ADMIN"
    virtual std::string linkedAccount() const { return "-"; }

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }
    bool locked() const { return locked_; }
    int failedAttempts() const { return failed_; }
    int attemptsLeft() const { return MAX_ATTEMPTS - failed_; }

    bool verifyPin(const std::string& pin) const;
    void setPin(const std::string& pin);
    void recordFailure();                                       // locks the user at MAX_ATTEMPTS
    void resetFailures() { failed_ = 0; }
    void unlock() { locked_ = false; failed_ = 0; }

    // Repository<T> contract
    std::string key() const { return id_; }
    std::string toCsv() const;
    static std::unique_ptr<User> fromCsv(const std::string& line);
    static const char* header() { return "id,role,name,salt,hash,failed,locked,accNo"; }
};

class Customer : public User {
    std::string accNo_;
public:
    Customer(std::string id, std::string name, const std::string& pin, std::string accNo)
        : User(std::move(id), std::move(name), pin), accNo_(std::move(accNo)) {}
    Customer(std::string id, std::string name, std::string salt, std::string hash, int failed, bool locked, std::string accNo)
        : User(std::move(id), std::move(name), std::move(salt), std::move(hash), failed, locked), accNo_(std::move(accNo)) {}
    std::string role() const override { return "CUSTOMER"; }
    std::string linkedAccount() const override { return accNo_; }
};

class Admin : public User {
public:
    using User::User;
    std::string role() const override { return "ADMIN"; }
};
