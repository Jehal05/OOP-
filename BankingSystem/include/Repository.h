// Repository.h - generic CSV-backed store: Repository<T>.
// T must provide:  std::string key() const;  std::string toCsv() const;
//                  static std::unique_ptr<T> fromCsv(const std::string&);  static const char* header();
// Demonstrates templates, unique_ptr ownership (move-only => Rule of 5), fstream and
// seekg/seekp/tellg/tellp for in-place record patching.
#pragma once
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include "Exceptions.h"

template <typename T>
class Repository {
    std::string path_;
    std::vector<std::unique_ptr<T>> items_;
    size_t skipped_ = 0;

    static std::string stripCR(std::string s) { if (!s.empty() && s.back() == '\r') s.pop_back(); return s; }
    void ensureDir() const {
        auto parent = std::filesystem::path(path_).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
    }
public:
    Repository() = default;
    explicit Repository(std::string path) : path_(std::move(path)) {}
    ~Repository() = default;
    // Rule of 5: owns unique_ptrs => copying is disabled, moving is allowed.
    Repository(const Repository&) = delete;
    Repository& operator=(const Repository&) = delete;
    Repository(Repository&&) noexcept = default;
    Repository& operator=(Repository&&) noexcept = default;

    void setPath(std::string p) { path_ = std::move(p); }
    const std::string& path() const { return path_; }
    size_t skippedLines() const { return skipped_; }
    bool fileExists() const { return std::filesystem::exists(path_); }

    // ---------------------------------------------------------- file I/O
    void load() {
        items_.clear(); skipped_ = 0;
        std::ifstream in(path_, std::ios::binary);
        if (!in) return;                                   // first run: nothing yet
        std::string line;
        std::getline(in, line);                            // header
        while (std::getline(in, line)) {
            line = stripCR(line);
            if (line.empty()) continue;
            try {
                auto item = T::fromCsv(line);
                if (item) items_.push_back(std::move(item)); else ++skipped_;
            } catch (const std::exception&) { ++skipped_; }
        }
    }

    void saveAll() const {
        ensureDir();
        std::ofstream out(path_, std::ios::binary | std::ios::trunc);
        if (!out) throw FileIOException("cannot write " + path_);
        out << T::header() << '\n';
        for (const auto& it : items_) out << it->toCsv() << '\n';
    }

    // Append just one record to the end of the file (cheap for the ever-growing transaction log).
    void appendToFile(const T& item) const {
        ensureDir();
        bool fresh = !std::filesystem::exists(path_) || std::filesystem::file_size(path_) == 0;
        std::ofstream out(path_, std::ios::binary | std::ios::app);
        if (!out) throw FileIOException("cannot append to " + path_);
        if (fresh) out << T::header() << '\n';
        out << item.toCsv() << '\n';
    }

    // Rewrite one record IN PLACE using seekg/tellg to locate it and seekp to overwrite it.
    // Works when the new line has the same byte length (e.g. fixed-width balances);
    // otherwise falls back to rewriting the whole file.
    bool patch(const std::string& key) const {
        const T* item = find(key);
        if (!item) return false;
        const std::string fresh = item->toCsv();
        std::fstream io(path_, std::ios::in | std::ios::out | std::ios::binary);
        if (!io) { saveAll(); return true; }
        std::string line;
        std::getline(io, line);                            // skip header
        while (true) {
            std::streampos start = io.tellg();             // where this record begins
            if (!std::getline(io, line)) break;
            std::string clean = stripCR(line);
            if (clean.compare(0, key.size() + 1, key + ",") == 0) {
                if (clean.size() == fresh.size()) {
                    io.clear();
                    io.seekp(start);                       // jump back and overwrite
                    io.write(fresh.data(), (std::streamsize)fresh.size());
                    io.flush();
                    return true;
                }
                break;                                     // length changed: rewrite everything
            }
        }
        io.close();
        saveAll();
        return true;
    }

    // ---------------------------------------------------------- collection API
    T* add(std::unique_ptr<T> item) { items_.push_back(std::move(item)); return items_.back().get(); }
    T* addAndAppend(std::unique_ptr<T> item) { T* p = add(std::move(item)); appendToFile(*p); return p; }

    T* find(const std::string& key) const {
        auto it = std::find_if(items_.begin(), items_.end(), [&](const std::unique_ptr<T>& p) { return p->key() == key; });
        return it == items_.end() ? nullptr : it->get();
    }
    bool remove(const std::string& key) {
        auto it = std::remove_if(items_.begin(), items_.end(), [&](const std::unique_ptr<T>& p) { return p->key() == key; });
        bool any = it != items_.end();
        items_.erase(it, items_.end());
        return any;
    }
    template <typename Pred>
    std::vector<T*> where(Pred pred) const {
        std::vector<T*> out;
        for (const auto& p : items_) if (pred(*p)) out.push_back(p.get());
        return out;
    }
    const std::vector<std::unique_ptr<T>>& all() const { return items_; }
    size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
};
