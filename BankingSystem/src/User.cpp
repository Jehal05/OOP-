#include "User.h"
#include "Utils.h"

User::User(std::string id, std::string name, const std::string& pin)
    : id_(std::move(id)), name_(util::sanitize(name)), salt_(util::randomSalt()), hash_(util::hashPin(pin, salt_)),
      failed_(0), locked_(false) {}

User::User(std::string id, std::string name, std::string salt, std::string hash, int failed, bool locked)
    : id_(std::move(id)), name_(util::sanitize(name)), salt_(std::move(salt)), hash_(std::move(hash)),
      failed_(failed), locked_(locked) {}

bool User::verifyPin(const std::string& pin) const { return util::hashPin(pin, salt_) == hash_; }

void User::setPin(const std::string& pin) {
    salt_ = util::randomSalt();
    hash_ = util::hashPin(pin, salt_);
}

void User::recordFailure() {
    if (++failed_ >= MAX_ATTEMPTS) locked_ = true;
}

std::string User::toCsv() const {
    return util::join({id_, role(), name_, salt_, hash_, std::to_string(failed_), locked_ ? "1" : "0", linkedAccount()}, ',');
}

std::unique_ptr<User> User::fromCsv(const std::string& line) {
    auto p = util::split(line, ',');
    if (p.size() < 8) return nullptr;
    int failed = std::stoi(p[5]);
    bool locked = p[6] == "1";
    if (p[1] == "CUSTOMER") return std::make_unique<Customer>(p[0], p[2], p[3], p[4], failed, locked, p[7]);
    if (p[1] == "ADMIN") return std::make_unique<Admin>(p[0], p[2], p[3], p[4], failed, locked);
    return nullptr;
}
