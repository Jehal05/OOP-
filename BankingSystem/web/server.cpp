// Nova Bank - local web server.
// A thin JSON/HTTP layer over the SAME Bank classes the console app uses. No business rule lives here:
// every action goes through Bank, which enforces limits, fraud screening, audit logging and persistence.
//
// Security notes: binds to 127.0.0.1 only; HttpOnly + SameSite=Strict session cookie; JSON-only POSTs;
// Host-header check (DNS-rebinding guard); every handler is serialized by one mutex because the CSV
// repositories are single-writer. This is a learning project, not a hardened production server.
#include "httplib.h"
#include "Bank.h"
#include "Utils.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <tuple>
#include <algorithm>
#include <unordered_map>

// ------------------------------------------------------------------ tiny JSON helpers
static std::string q(const std::string& s) {                  // JSON string literal
    std::string o = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += (char)c;
        }
    }
    return o + "\"";
}
static std::string n2(double v) { char b[64]; std::snprintf(b, sizeof b, "%.2f", v); return b; }
static std::string b2(bool v) { return v ? "true" : "false"; }
static std::string arr(const std::vector<std::string>& items) {
    std::string o = "[";
    for (size_t i = 0; i < items.size(); ++i) o += (i ? "," : "") + items[i];
    return o + "]";
}
// {"a":1,"b":"x"} from alternating key / already-encoded value pairs
static std::string obj(std::initializer_list<std::pair<const char*, std::string>> kv) {
    std::string o = "{";
    bool first = true;
    for (const auto& p : kv) { o += (first ? "" : ",") + q(p.first) + ":" + p.second; first = false; }
    return o + "}";
}

static std::map<std::string, std::string> parseJson(const std::string& s) {    // flat objects only
    std::map<std::string, std::string> out;
    size_t i = 0;
    auto skip = [&] { while (i < s.size() && std::isspace((unsigned char)s[i])) ++i; };
    auto str = [&] {
        std::string r; ++i;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i + 1 < s.size()) {
                ++i;
                switch (s[i]) { case 'n': r += '\n'; break; case 't': r += '\t'; break; case 'u': r += '?'; i += 4; break; default: r += s[i]; }
            } else r += s[i];
            ++i;
        }
        ++i; return r;
    };
    skip();
    if (i >= s.size() || s[i] != '{') return out;
    ++i;
    for (;;) {
        skip();
        if (i >= s.size() || s[i] == '}') break;
        if (s[i] == ',') { ++i; continue; }
        if (s[i] != '"') break;
        std::string k = str(); skip();
        if (i < s.size() && s[i] == ':') ++i;
        skip();
        std::string v;
        if (i < s.size() && s[i] == '"') v = str();
        else { while (i < s.size() && s[i] != ',' && s[i] != '}') v += s[i++]; v = util::trim(v); }
        out[k] = v;
    }
    return out;
}

// ------------------------------------------------------------------ request context
struct Session { std::string userId, role, accNo, name; User* user; time_t last; };
enum class Who { Public, Customer, Admin };

struct Ctx {
    const httplib::Request& req;
    Bank& bank;
    Session* s;
    std::map<std::string, std::string> body;
    std::string get(const std::string& k) const {
        auto it = body.find(k);
        if (it != body.end()) return it->second;
        return req.has_param(k) ? req.get_param_value(k) : "";
    }
    bool has(const std::string& k) const { return !get(k).empty(); }
    double amount(const std::string& k, bool allowZero = false) const {
        std::string v = util::trim(get(k));
        char* end = nullptr;
        double d = std::strtod(v.c_str(), &end);
        if (v.empty() || *end || !std::isfinite(d)) throw InvalidAmountException("please enter a valid number");
        if (d < 0 || (!allowZero && d == 0)) throw InvalidAmountException("amount must be greater than zero");
        if (d > 1e9) throw InvalidAmountException("amount is too large");
        if (std::fabs(d * 100 - std::round(d * 100)) > 1e-6) throw InvalidAmountException("use at most 2 decimal places");
        return std::round(d * 100) / 100;
    }
    int integer(const std::string& k) const {
        std::string v = util::trim(get(k));
        char* end = nullptr;
        long n = std::strtol(v.c_str(), &end, 10);
        if (v.empty() || *end || n < -1000000 || n > 1000000) throw BankException("please enter a whole number for " + k);
        return (int)n;
    }
    const std::string& acc() const { return s->accNo; }
};

static Category parseCategory(const std::string& s) {
    std::string l = util::lower(s);
    for (Category c : allCategories()) if (util::lower(toString(c)) == l) return c;
    return Category::Other;
}
static std::optional<TxType> parseType(const std::string& s) {
    std::string l = util::lower(s);
    for (TxType t : allTxTypes()) if (util::lower(toString(t)) == l) return t;
    return std::nullopt;
}

// ------------------------------------------------------------------ serializers
static std::string txJson(const Transaction& t) {
    return obj({{"ref", q(t.refId())}, {"acc", q(t.accNo())}, {"type", q(toString(t.type()))},
                {"category", q(t.credit() ? "-" : toString(t.category()))}, {"amount", n2(t.amount())},
                {"credit", b2(t.credit())}, {"balanceAfter", n2(t.balanceAfter())},
                {"time", q(util::fmtDateTime(t.timestamp()))}, {"note", q(t.note())}});
}
static std::string accJson(const Account& a) {
    return obj({{"accNo", q(a.accNo())}, {"holder", q(a.holder())}, {"type", q(a.typeName())}, {"typeCode", q(a.typeCode())},
                {"balance", n2(a.balance())}, {"spendable", n2(a.spendable())}, {"rate", n2(a.annualRate())},
                {"minBalance", n2(a.minimumBalance())}, {"rules", q(a.describe())},
                {"opened", q(util::fmtDate(a.openedOn()))}, {"active", b2(a.active())}});
}
static std::string loanJson(const Loan& l, const Bank& bank) {
    const Account* a = bank.findAccount(l.accNo());
    return obj({{"id", q(l.id())}, {"acc", q(l.accNo())}, {"holder", q(a ? a->holder() : "-")}, {"type", q(l.typeName())},
                {"typeCode", q(l.typeCode())}, {"principal", n2(l.principal())}, {"months", std::to_string(l.months())},
                {"rate", n2(l.annualRate())}, {"status", q(toString(l.status()))}, {"paid", std::to_string(l.paid())},
                {"emi", n2(l.emi())}, {"outstanding", n2(l.outstanding())}, {"next", n2(l.nextInstallment())},
                {"remaining", std::to_string(l.remainingInstallments())}, {"note", q(l.note())},
                {"applied", q(util::fmtDate(l.appliedOn()))}});
}
static std::string goalJson(const SavingsGoal& g) {
    time_t now = util::now();
    return obj({{"id", q(g.id())}, {"name", q(g.name())}, {"target", n2(g.target())}, {"saved", n2(g.saved())},
                {"fraction", n2(g.fraction())}, {"status", q(g.status(now))}, {"deadline", q(util::fmtDate(g.deadline()))},
                {"monthly", n2(g.requiredMonthly(now))}});
}
static std::string alertJson(const Alert& a, const Bank& bank) {
    const Account* acc = bank.findAccount(a.accNo());
    return obj({{"id", q(a.id())}, {"acc", q(a.accNo())}, {"holder", q(acc ? acc->holder() : "-")}, {"ref", q(a.refId())},
                {"rule", q(a.rule())}, {"message", q(a.message())}, {"time", q(util::fmtDateTime(a.timestamp()))},
                {"reviewed", b2(a.reviewed())}, {"seen", b2(a.seen())}});
}
static std::string txList(const std::vector<Transaction>& v, size_t cap = 500) {
    std::vector<std::string> o;
    for (size_t i = 0; i < v.size() && i < cap; ++i) o.push_back(txJson(v[i]));
    return arr(o);
}

static StatementFilter filterFrom(const Ctx& c) {
    StatementFilter f;
    time_t t;
    if (c.has("from")) { if (!util::parseDate(c.get("from"), t)) throw BankException("Use YYYY-MM-DD for the 'from' date."); f.from = t; }
    if (c.has("to")) { if (!util::parseDate(c.get("to"), t)) throw BankException("Use YYYY-MM-DD for the 'to' date."); f.to = util::addDays(t, 1) - 1; }
    if (c.has("type")) { auto ty = parseType(c.get("type")); if (!ty) throw BankException("Unknown transaction type."); f.type = ty; }
    if (c.has("category")) f.category = parseCategory(c.get("category"));
    if (c.has("min")) f.minAmount = c.amount("min", true);
    if (c.has("max")) f.maxAmount = c.amount("max", true);
    return f;
}

// ------------------------------------------------------------------ main
int main(int argc, char** argv) {
    std::string dataDir = std::getenv("BANK_DATA_DIR") ? std::getenv("BANK_DATA_DIR") : "data";
    std::string webDir = "web/public";
    int port = 8080;
    int timeoutSec = std::getenv("BANK_TIMEOUT_SEC") ? std::max(30, std::atoi(std::getenv("BANK_TIMEOUT_SEC"))) : 600;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--data") && i + 1 < argc) dataDir = argv[++i];
        else if (!std::strcmp(argv[i], "--web") && i + 1 < argc) webDir = argv[++i];
        else if (!std::strcmp(argv[i], "--port") && i + 1 < argc) port = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--help")) {
            std::cout << "Nova Bank web server\nUsage: bankweb [--port 8080] [--data DIR] [--web DIR]\nOpen http://localhost:PORT\n";
            return 0;
        }
    }
    Bank bank(dataDir);
    try { bank.load(); } catch (const std::exception& e) { std::cerr << "Cannot load data: " << e.what() << "\n"; return 1; }

    std::mutex mu;
    std::unordered_map<std::string, Session> sessions;
    std::random_device rd;
    auto newToken = [&] {
        std::string t; char b[3];
        for (int i = 0; i < 24; ++i) { std::snprintf(b, sizeof b, "%02x", (unsigned)(rd() & 0xFF)); t += b; }
        return t;
    };
    auto cookieOf = [](const httplib::Request& r) {
        std::string h = r.get_header_value("Cookie");
        size_t p = h.find("sid=");
        if (p == std::string::npos) return std::string();
        size_t e = h.find(';', p);
        return h.substr(p + 4, e == std::string::npos ? std::string::npos : e - p - 4);
    };

    httplib::Server svr;
    svr.set_payload_max_length(64 * 1024);

    svr.set_pre_routing_handler([&](const httplib::Request& req, httplib::Response& res) {
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("X-Frame-Options", "DENY");
        res.set_header("Referrer-Policy", "no-referrer");
        res.set_header("Cache-Control", "no-store");
        res.set_header("Content-Security-Policy", "default-src 'self'; style-src 'self' 'unsafe-inline'; img-src 'self' data:");
        std::string host = req.get_header_value("Host");
        bool ok = host.rfind("localhost", 0) == 0 || host.rfind("127.0.0.1", 0) == 0 || host.rfind("[::1]", 0) == 0;
        if (!ok) { res.status = 403; res.set_content("{\"error\":\"Forbidden host\"}", "application/json"); return httplib::Server::HandlerResponse::Handled; }
        return httplib::Server::HandlerResponse::Unhandled;
    });

    using Handler = std::function<std::string(Ctx&)>;
    auto reg = [&](const std::string& method, const std::string& path, Who who, Handler h) {
        auto fn = [&, method, path, who, h](const httplib::Request& req, httplib::Response& res) {
            std::lock_guard<std::mutex> lock(mu);
            res.set_header("Content-Type", "application/json");
            try {
                if (method == "POST" && req.get_header_value("Content-Type").find("application/json") == std::string::npos)
                    throw BankException("Content-Type must be application/json.");
                time_t now = util::now();
                for (auto it = sessions.begin(); it != sessions.end();)
                    it = (now - it->second.last > timeoutSec) ? sessions.erase(it) : std::next(it);
                Session* s = nullptr;
                auto it = sessions.find(cookieOf(req));
                if (it != sessions.end()) { s = &it->second; s->last = now; }
                if (who != Who::Public) {
                    if (!s) { res.status = 401; res.set_content("{\"error\":\"Please log in (your session may have expired).\"}", "application/json"); return; }
                    if ((who == Who::Admin) != (s->role == "ADMIN")) { res.status = 403; res.set_content("{\"error\":\"Not allowed for this role.\"}", "application/json"); return; }
                }
                Ctx c{req, bank, s, method == "POST" ? parseJson(req.body) : std::map<std::string, std::string>{}};
                // login / logout manage the cookie themselves, so they reach into `res` through these members
                res.set_content(h(c), "application/json");
                if (path == "/api/login") {
                    std::string tok = newToken();
                    User& u = bank.login(c.get("id"), c.get("pin"), util::upper(c.get("role")) == "ADMIN" ? "ADMIN" : "CUSTOMER");
                    sessions[tok] = Session{u.id(), u.role(), u.role() == "CUSTOMER" ? u.linkedAccount() : "", u.name(), &u, now};
                    res.set_header("Set-Cookie", "sid=" + tok + "; HttpOnly; SameSite=Strict; Path=/");
                    res.set_content(obj({{"role", q(u.role())}, {"name", q(u.name())}, {"id", q(u.id())},
                                         {"accNo", q(sessions[tok].accNo)}}), "application/json");
                } else if (path == "/api/logout" && s) {
                    bank.logAudit(s->userId, "LOGOUT", s->role);
                    sessions.erase(cookieOf(req));
                    res.set_header("Set-Cookie", "sid=; Max-Age=0; Path=/");
                }
            } catch (const AccountLockedException& e) { res.status = 423; res.set_content(obj({{"error", q(e.what())}}), "application/json");
            } catch (const AuthenticationException& e) { res.status = 401; res.set_content(obj({{"error", q(e.what())}}), "application/json");
            } catch (const BankException& e) { res.status = 400; res.set_content(obj({{"error", q(e.what())}}), "application/json");
            } catch (const std::exception& e) { res.status = 500; res.set_content(obj({{"error", q(std::string("Unexpected error: ") + e.what())}}), "application/json"); }
        };
        if (method == "GET") svr.Get(path, fn); else svr.Post(path, fn);
    };

    // ============================================================== session
    reg("POST", "/api/login", Who::Public, [](Ctx&) { return std::string("{}"); });   // replaced after bank.login succeeds
    reg("POST", "/api/logout", Who::Public, [](Ctx&) { return std::string("{\"ok\":true}"); });
    reg("GET", "/api/me", Who::Public, [](Ctx& c) {
        if (!c.s) return std::string("{\"loggedIn\":false}");
        return obj({{"loggedIn", "true"}, {"role", q(c.s->role)}, {"name", q(c.s->name)}, {"id", q(c.s->userId)}, {"accNo", q(c.s->accNo)}});
    });

    // ============================================================== customer
    reg("GET", "/api/dashboard", Who::Customer, [](Ctx& c) {
        Account& a = c.bank.account(c.acc());
        auto h = c.bank.history(a.accNo());
        std::vector<std::string> recent, trend;
        for (size_t i = h.size(); i-- > 0 && recent.size() < 6;) recent.push_back(txJson(h[i]));
        for (size_t i = h.size() > 14 ? h.size() - 14 : 0; i < h.size(); ++i) trend.push_back(n2(h[i].balanceAfter()));
        std::string month = util::monthKey(util::now());
        double in = 0, out = 0;
        for (const auto& t : h) if (util::monthKey(t.timestamp()) == month) (t.credit() ? in : out) += t.amount();
        int active = 0;
        for (const Loan* l : c.bank.loansOf(a.accNo())) if (l->status() == LoanStatus::Active) ++active;
        return obj({{"account", accJson(a)}, {"recent", arr(recent)}, {"trend", arr(trend)}, {"monthIn", n2(in)}, {"monthOut", n2(out)},
                    {"unseenAlerts", std::to_string(c.bank.unseenAlerts(a.accNo()).size())},
                    {"goals", std::to_string(c.bank.goalsOf(a.accNo()).size())}, {"activeLoans", std::to_string(active)},
                    {"name", q(c.s->name)}});
    });
    reg("POST", "/api/deposit", Who::Customer, [](Ctx& c) {
        const Transaction& t = c.bank.deposit(c.acc(), c.amount("amount"), util::cleanText(c.get("note")).empty() ? "Cash deposit" : util::cleanText(c.get("note")));
        return obj({{"tx", txJson(t)}, {"balance", n2(c.bank.account(c.acc()).balance())}});
    });
    reg("GET", "/api/fdfee", Who::Customer, [](Ctx& c) {
        return obj({{"fee", n2(c.bank.fdWithdrawalFee(c.acc(), c.amount("amount")))}});
    });
    reg("POST", "/api/withdraw", Who::Customer, [](Ctx& c) {
        const Transaction& t = c.bank.withdraw(c.acc(), c.amount("amount"), parseCategory(c.get("category")), util::cleanText(c.get("note")));
        return obj({{"tx", txJson(t)}, {"balance", n2(c.bank.account(c.acc()).balance())}});
    });
    reg("POST", "/api/transfer", Who::Customer, [](Ctx& c) {
        std::string to = util::upper(util::trim(c.get("to")));
        if (c.has("payee")) {
            const Beneficiary* b = c.bank.findPayee(c.acc(), c.get("payee"));
            if (!b) throw BankException("Payee not found.");
            to = b->payeeAcc();
        }
        const Transaction& t = c.bank.transfer(c.acc(), to, c.amount("amount"), parseCategory(c.get("category")), util::cleanText(c.get("note")));
        return obj({{"tx", txJson(t)}, {"balance", n2(c.bank.account(c.acc()).balance())}});
    });
    reg("GET", "/api/lookup", Who::Customer, [](Ctx& c) {      // confirm the recipient name before sending money
        const Account* a = c.bank.findAccount(util::upper(util::trim(c.get("acc"))));
        if (!a || !a->active()) throw AccountNotFoundException(c.get("acc"));
        return obj({{"accNo", q(a->accNo())}, {"holder", q(a->holder())}, {"masked", q(util::maskAccount(a->accNo()))}});
    });
    reg("GET", "/api/receipt", Who::Customer, [](Ctx& c) {
        const Transaction* t = c.bank.findTransaction(c.get("ref"));
        if (!t || t->accNo() != c.acc()) throw BankException("No receipt found for that reference.");
        return txJson(*t);
    });

    auto buildStatement = [](Ctx& c) {
        Account& a = c.bank.account(c.acc());
        Statement st(a.accNo(), a.holder(), c.bank.history(a.accNo()));
        StatementFilter f = filterFrom(c);
        st.filter(f);
        st.sort(c.get("sort") == "amount" ? SortKey::Amount : SortKey::Date, c.get("dir") == "asc");
        return std::make_tuple(std::move(st), f, a.balance());
    };
    reg("GET", "/api/statement", Who::Customer, [buildStatement](Ctx& c) {
        auto [st, f, bal] = buildStatement(c);
        auto s = st.summary();
        return obj({{"rows", txList(st.rows())}, {"count", std::to_string(st.rows().size())}, {"filters", q(f.describe())},
                    {"summary", obj({{"credits", n2(s.credits)}, {"debits", n2(s.debits)}, {"net", n2(s.net)},
                                     {"average", n2(s.average)}, {"largest", n2(s.largest)}, {"count", std::to_string(s.count)}})},
                    {"balance", n2(bal)}});
    });
    svr.Get("/api/statement.txt", [&, buildStatement](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(mu);
        auto it = sessions.find(cookieOf(req));
        if (it == sessions.end() || it->second.role != "CUSTOMER") { res.status = 401; res.set_content("Please log in.", "text/plain"); return; }
        try {
            Ctx c{req, bank, &it->second, {}};
            auto [st, f, bal] = buildStatement(c);
            std::string path = st.exportText(bank.dataDir() + "/../statements", f, bal);
            std::ifstream in(path, std::ios::binary);
            std::stringstream ss; ss << in.rdbuf();
            bank.logAudit(c.acc(), "STATEMENT_EXPORT", "web");
            res.set_header("Content-Disposition", "attachment; filename=\"statement_" + c.acc() + ".txt\"");
            res.set_content(ss.str(), "text/plain; charset=utf-8");
        } catch (const std::exception& e) { res.status = 400; res.set_content(e.what(), "text/plain"); }
    });
    reg("GET", "/api/spending", Who::Customer, [](Ctx& c) {
        auto slices = SpendingAnalyzer::analyze(c.bank.history(c.acc()), c.get("month"));
        std::vector<std::string> o;
        double total = 0;
        for (const auto& s : slices) { o.push_back(obj({{"category", q(toString(s.category))}, {"total", n2(s.total)}, {"percent", n2(s.percent)}})); total += s.total; }
        return obj({{"slices", arr(o)}, {"total", n2(total)}});
    });

    // goals
    reg("GET", "/api/goals", Who::Customer, [](Ctx& c) {
        std::vector<std::string> o;
        for (const SavingsGoal* g : c.bank.goalsOf(c.acc())) o.push_back(goalJson(*g));
        return arr(o);
    });
    reg("POST", "/api/goals", Who::Customer, [](Ctx& c) {
        time_t d;
        if (!util::parseDate(c.get("deadline"), d)) throw BankException("Use YYYY-MM-DD for the deadline.");
        return goalJson(c.bank.createGoal(c.acc(), c.get("name"), c.amount("target"), d));
    });
    reg("POST", "/api/goals/contribute", Who::Customer, [](Ctx& c) {
        bool mine = false;
        for (const SavingsGoal* g : c.bank.goalsOf(c.acc())) if (g->id() == util::upper(c.get("id"))) mine = true;
        if (!mine) throw BankException("Goal not found.");
        const Transaction& t = c.bank.contributeToGoal(util::upper(c.get("id")), c.amount("amount"));
        return obj({{"tx", txJson(t)}, {"balance", n2(c.bank.account(c.acc()).balance())}});
    });

    // loans
    reg("GET", "/api/loans", Who::Customer, [](Ctx& c) {
        std::vector<std::string> o;
        for (const Loan* l : c.bank.loansOf(c.acc())) o.push_back(loanJson(*l, c.bank));
        return arr(o);
    });
    reg("GET", "/api/emi", Who::Customer, [](Ctx& c) {
        auto l = Loan::create(util::upper(c.get("type")), "", "-", c.amount("principal"), c.integer("months"), LoanStatus::Pending, 0, util::now());
        if (!l) throw LoanException("Unknown loan type.");
        auto sch = l->schedule();
        double interest = 0, total = 0;
        for (const auto& i : sch) { interest += i.interest; total += i.emi; }
        std::vector<std::string> rows;
        for (const auto& i : sch) rows.push_back(obj({{"month", std::to_string(i.month)}, {"emi", n2(i.emi)}, {"interest", n2(i.interest)}, {"principal", n2(i.principal)}, {"balance", n2(i.balance)}}));
        return obj({{"type", q(l->typeName())}, {"rate", n2(l->annualRate())}, {"emi", n2(sch.empty() ? 0 : sch.front().emi)}, {"totalInterest", n2(interest)},
                    {"totalPayable", n2(total)}, {"maxMonths", std::to_string(l->maxMonths())}, {"maxPrincipal", n2(l->maxPrincipal())}, {"schedule", arr(rows)}});
    });
    reg("POST", "/api/loans/apply", Who::Customer, [](Ctx& c) {
        return loanJson(c.bank.applyLoan(c.acc(), util::upper(c.get("type")), c.amount("principal"), c.integer("months")), c.bank);
    });
    auto ownLoan = [](Ctx& c) -> Loan& {
        Loan* l = c.bank.findLoan(util::upper(c.get("id")));
        if (!l || l->accNo() != c.acc()) throw LoanException("Loan not found.");
        return *l;
    };
    reg("GET", "/api/loans/schedule", Who::Customer, [ownLoan](Ctx& c) {
        Loan& l = ownLoan(c);
        std::vector<std::string> rows;
        for (const auto& i : l.schedule()) rows.push_back(obj({{"month", std::to_string(i.month)}, {"emi", n2(i.emi)}, {"interest", n2(i.interest)}, {"principal", n2(i.principal)}, {"balance", n2(i.balance)}, {"paid", b2(i.month <= l.paid())}}));
        return obj({{"loan", loanJson(l, c.bank)}, {"schedule", arr(rows)}});
    });
    reg("POST", "/api/loans/pay", Who::Customer, [ownLoan](Ctx& c) {
        Loan& l = ownLoan(c);
        const Transaction& t = c.bank.payEMI(l.id());
        return obj({{"tx", txJson(t)}, {"loan", loanJson(l, c.bank)}, {"balance", n2(c.bank.account(c.acc()).balance())}});
    });
    reg("POST", "/api/loans/prepay", Who::Customer, [ownLoan](Ctx& c) {
        Loan& l = ownLoan(c);
        const Transaction& t = c.bank.prepayLoan(l.id());
        return obj({{"tx", txJson(t)}, {"loan", loanJson(l, c.bank)}, {"balance", n2(c.bank.account(c.acc()).balance())}});
    });

    // payees
    reg("GET", "/api/payees", Who::Customer, [](Ctx& c) {
        std::vector<std::string> o;
        for (const Beneficiary* b : c.bank.payeesOf(c.acc()))
            o.push_back(obj({{"nickname", q(b->nickname())}, {"acc", q(b->payeeAcc())}, {"name", q(b->payeeName())}, {"added", q(util::fmtDate(b->addedOn()))},
                             {"isNew", b2(util::now() - b->addedOn() < 86400)}}));
        return arr(o);
    });
    reg("POST", "/api/payees", Who::Customer, [](Ctx& c) {
        const Beneficiary& b = c.bank.addPayee(c.acc(), c.get("nickname"), c.get("account"));
        return obj({{"nickname", q(b.nickname())}, {"acc", q(b.payeeAcc())}, {"name", q(b.payeeName())}});
    });
    reg("POST", "/api/payees/remove", Who::Customer, [](Ctx& c) {
        c.bank.removePayee(c.acc(), c.get("nickname"));
        return std::string("{\"ok\":true}");
    });

    // alerts, pin, simulator
    reg("GET", "/api/alerts", Who::Customer, [](Ctx& c) {
        std::vector<std::string> o;
        for (const Alert* a : c.bank.alertsOf(c.acc())) o.push_back(alertJson(*a, c.bank));
        return arr(o);
    });
    reg("POST", "/api/alerts/seen", Who::Customer, [](Ctx& c) { c.bank.markAlertsSeen(c.acc()); return std::string("{\"ok\":true}"); });
    reg("POST", "/api/pin", Who::Customer, [](Ctx& c) {
        c.bank.changePin(*c.s->user, c.get("oldPin"), c.get("newPin"));
        return std::string("{\"ok\":true}");
    });
    reg("GET", "/api/simulate", Who::Customer, [](Ctx& c) {
        double p = c.amount("principal");
        int years = c.integer("years");
        if (years < 1 || years > 20) throw BankException("Years must be between 1 and 20.");
        SavingsAccount sav("-", "-", p, 0, 4.0);
        StudentSavingsAccount stu("-", "-", p, 0, 4.5);
        FixedDepositAccount fd("-", "-", p, 0, 7.0, 12);
        auto rs = Bank::simulate(sav, p, years), rt = Bank::simulate(stu, p, years), rf = Bank::simulate(fd, p, years);
        std::vector<std::string> rows;
        for (int y = 0; y < years; ++y)
            rows.push_back(obj({{"year", std::to_string(y + 1)}, {"savings", n2(rs[(size_t)y].simple)}, {"student", n2(rt[(size_t)y].simple)},
                                {"fd", n2(rf[(size_t)y].compound)}, {"current", n2(p)}}));
        return obj({{"rows", arr(rows)}, {"principal", n2(p)}, {"years", std::to_string(years)}});
    });

    // ============================================================== admin
    reg("GET", "/api/admin/overview", Who::Admin, [](Ctx& c) {
        return obj({{"accounts", std::to_string(c.bank.allAccounts().size())}, {"deposits", n2(c.bank.totalDeposits())},
                    {"totalBalance", n2(c.bank.totalBalance())}, {"pendingLoans", std::to_string(c.bank.pendingLoans().size())},
                    {"alerts", std::to_string(c.bank.alertsAwaitingReview().size())}, {"locked", std::to_string(c.bank.lockedUsers().size())},
                    {"rules", std::to_string(c.bank.fraudRuleCount())}, {"name", q(c.s->name)}});
    });
    reg("GET", "/api/admin/accounts", Who::Admin, [](Ctx& c) {
        auto v = c.has("q") ? c.bank.searchAccounts(c.get("q")) : c.bank.allAccounts();
        std::vector<std::string> o;
        for (const Account* a : v) o.push_back(accJson(*a));
        return arr(o);
    });
    reg("POST", "/api/admin/accounts", Who::Admin, [](Ctx& c) {
        std::string code = util::upper(c.get("type"));
        double p1 = code == "STU" ? 4.5 : code == "CUR" ? 25000 : code == "FD" ? 7.0 : 4.0;
        double p2 = code == "FD" ? 12 : 1000;
        if (c.has("p1")) p1 = c.amount("p1", true);
        if (c.has("p2")) p2 = c.amount("p2", true);
        std::string no = c.bank.openAccount(c.s->userId, code, c.get("holder"), c.amount("deposit", true), c.get("pin"), p1, p2);
        return accJson(c.bank.account(no));
    });
    reg("POST", "/api/admin/accounts/update", Who::Admin, [](Ctx& c) {
        c.bank.updateHolder(c.s->userId, util::upper(c.get("acc")), c.get("holder"));
        return accJson(c.bank.account(util::upper(c.get("acc"))));
    });
    reg("POST", "/api/admin/accounts/close", Who::Admin, [](Ctx& c) {
        double payout = c.bank.closeAccount(c.s->userId, util::upper(c.get("acc")));
        return obj({{"payout", n2(payout)}});
    });
    reg("GET", "/api/admin/history", Who::Admin, [](Ctx& c) {
        std::string acc = util::upper(util::trim(c.get("acc")));
        const Account& a = c.bank.account(acc);
        auto h = c.bank.history(acc);
        std::reverse(h.begin(), h.end());
        return obj({{"account", accJson(a)}, {"rows", txList(h, 200)}});
    });
    reg("GET", "/api/admin/loans", Who::Admin, [](Ctx& c) {
        std::vector<std::string> o;
        for (const Loan* l : c.bank.pendingLoans()) o.push_back(loanJson(*l, c.bank));
        return arr(o);
    });
    reg("POST", "/api/admin/loans/approve", Who::Admin, [](Ctx& c) { c.bank.approveLoan(c.s->userId, util::upper(c.get("id"))); return std::string("{\"ok\":true}"); });
    reg("POST", "/api/admin/loans/reject", Who::Admin, [](Ctx& c) { c.bank.rejectLoan(c.s->userId, util::upper(c.get("id")), util::cleanText(c.get("reason"))); return std::string("{\"ok\":true}"); });
    reg("GET", "/api/admin/alerts", Who::Admin, [](Ctx& c) {
        std::vector<std::string> o;
        for (const Alert* a : c.bank.alertsAwaitingReview()) o.push_back(alertJson(*a, c.bank));
        return arr(o);
    });
    reg("POST", "/api/admin/alerts/review", Who::Admin, [](Ctx& c) { c.bank.markAlertReviewed(c.s->userId, util::upper(c.get("id"))); return std::string("{\"ok\":true}"); });
    reg("GET", "/api/admin/locked", Who::Admin, [](Ctx& c) {
        std::vector<std::string> o;
        for (const User* u : c.bank.lockedUsers()) o.push_back(obj({{"id", q(u->id())}, {"name", q(u->name())}, {"role", q(u->role())}}));
        return arr(o);
    });
    reg("POST", "/api/admin/unlock", Who::Admin, [](Ctx& c) { c.bank.unlockUser(c.s->userId, c.get("id")); return std::string("{\"ok\":true}"); });
    reg("POST", "/api/admin/interest", Who::Admin, [](Ctx& c) {
        std::vector<std::string> o;
        double total = 0;
        for (const auto& r : c.bank.applyMonthlyInterest(c.s->userId)) {
            o.push_back(obj({{"acc", q(r.accNo)}, {"holder", q(r.holder)}, {"amount", n2(r.amount)}, {"status", q(r.status)}}));
            total += r.amount;
        }
        return obj({{"results", arr(o)}, {"total", n2(total)}});
    });
    reg("GET", "/api/admin/reports", Who::Admin, [](Ctx& c) {
        std::vector<std::string> rich, dorm, months;
        for (const Account* a : c.bank.richest(5)) rich.push_back(accJson(*a));
        for (const auto& [a, t] : c.bank.dormant(30)) dorm.push_back(obj({{"acc", q(a->accNo())}, {"holder", q(a->holder())}, {"last", q(util::fmtDate(t))}}));
        for (const auto& [k, v] : c.bank.periodTotals(true)) months.push_back(obj({{"month", q(k)}, {"credits", n2(v.first)}, {"debits", n2(v.second)}}));
        return obj({{"richest", arr(rich)}, {"dormant", arr(dorm)}, {"months", arr(months)}, {"deposits", n2(c.bank.totalDeposits())}});
    });
    reg("GET", "/api/admin/audit", Who::Admin, [](Ctx& c) {
        auto lines = c.has("q") ? c.bank.audit().search(c.get("q")) : c.bank.audit().readAll();
        std::vector<std::string> o;
        size_t start = lines.size() > 150 ? lines.size() - 150 : 0;
        for (size_t i = lines.size(); i-- > start;) o.push_back(q(lines[i]));
        return obj({{"lines", arr(o)}, {"total", std::to_string(lines.size())}});
    });

    // ============================================================== static files
    if (!svr.set_mount_point("/", webDir)) { std::cerr << "Cannot find web folder '" << webDir << "'. Run from the BankingSystem folder or pass --web DIR.\n"; return 1; }

    std::cout << "\n  Nova Bank web server\n  Open  http://localhost:" << port << "\n  Data  " << dataDir
              << "\n  Press Ctrl+C to stop.\n\n";
    if (!svr.listen("127.0.0.1", port)) { std::cerr << "Cannot listen on port " << port << " (already in use? try --port 8090).\n"; return 1; }
    bank.saveAll();
    return 0;
}
