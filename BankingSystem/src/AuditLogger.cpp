#include "AuditLogger.h"
#include "Utils.h"
#include "Exceptions.h"
#include <fstream>
#include <filesystem>

void AuditLogger::log(const std::string& actor, const std::string& action, const std::string& details) const {
    auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent);
    std::ofstream out(path_, std::ios::app | std::ios::binary);
    if (!out) throw FileIOException("cannot write audit log " + path_);
    out << util::fmtDateTime(util::now()) << " | " << util::sanitize(actor) << " | " << util::sanitize(action)
        << " | " << util::sanitize(details) << '\n';
}

std::vector<std::string> AuditLogger::readAll() const {
    std::vector<std::string> lines;
    std::ifstream in(path_, std::ios::binary);
    std::string l;
    while (std::getline(in, l)) if (!l.empty()) lines.push_back(l.back() == '\r' ? l.substr(0, l.size() - 1) : l);
    return lines;
}

std::vector<std::string> AuditLogger::search(const std::string& keyword) const {
    std::vector<std::string> hits;
    std::string k = util::lower(keyword);
    for (const auto& l : readAll())
        if (util::lower(l).find(k) != std::string::npos) hits.push_back(l);
    return hits;
}
