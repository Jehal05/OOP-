// AuditLogger.h - append-only audit trail:  timestamp | actor | action | details
#pragma once
#include <string>
#include <vector>

class AuditLogger {
    std::string path_;
public:
    AuditLogger() = default;
    explicit AuditLogger(std::string path) : path_(std::move(path)) {}
    void setPath(std::string p) { path_ = std::move(p); }
    // The file is only ever opened with ios::app, so past entries can never be edited by the app.
    void log(const std::string& actor, const std::string& action, const std::string& details = "") const;
    std::vector<std::string> readAll() const;
    std::vector<std::string> search(const std::string& keyword) const;
};
