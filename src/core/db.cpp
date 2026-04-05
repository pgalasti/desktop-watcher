#include "core/db.hpp"
#include <stdexcept>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <fmt/format.h>

namespace DesktopWatcher {

// ── Helpers ──────────────────────────────────────────────────────────────────

Database::Database(const std::string& path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    if (sqlite3_open(path.c_str(), &m_pDb) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(m_pDb);
        sqlite3_close(m_pDb);
        throw std::runtime_error(fmt::format("Failed to open database '{}': {}", path, msg));
    }
    Exec("PRAGMA journal_mode=WAL;");
    Exec("PRAGMA foreign_keys=ON;");
}

Database::~Database() {
    if (m_pDb) sqlite3_close(m_pDb);
}

Database::Database(Database&& other) noexcept : m_pDb(other.m_pDb) {
    other.m_pDb = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        if (m_pDb) sqlite3_close(m_pDb);
        m_pDb       = other.m_pDb;
        other.m_pDb = nullptr;
    }
    return *this;
}

void Database::Exec(const std::string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(m_pDb, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg(err);
        sqlite3_free(err);
        throw std::runtime_error(fmt::format("SQL error: {}", msg));
    }
}

std::string Database::NowISO() const {
    auto now = std::chrono::system_clock::now();
    auto t   = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

std::string Database::ColText(sqlite3_stmt* stmt, int col) {
    const unsigned char* raw = sqlite3_column_text(stmt, col);
    return raw ? reinterpret_cast<const char*>(raw) : "";
}

// ── Schema ───────────────────────────────────────────────────────────────────

void Database::InitSchema() {
    Exec(R"(
        CREATE TABLE IF NOT EXISTS sessions (
            id           INTEGER PRIMARY KEY AUTOINCREMENT,
            app_name     TEXT NOT NULL,
            window_title TEXT,
            category     TEXT DEFAULT 'uncategorized',
            started_at   TEXT NOT NULL,
            ended_at     TEXT,
            duration_s   INTEGER
        );
    )");
    Exec(R"(
        CREATE TABLE IF NOT EXISTS categories (
            id    INTEGER PRIMARY KEY AUTOINCREMENT,
            name  TEXT UNIQUE NOT NULL,
            color TEXT
        );
    )");
    Exec(R"(
        CREATE TABLE IF NOT EXISTS rules (
            id          INTEGER PRIMARY KEY AUTOINCREMENT,
            pattern     TEXT NOT NULL,
            match_field TEXT DEFAULT 'app_name',
            category    TEXT NOT NULL,
            priority    INTEGER DEFAULT 0
        );
    )");
}

// ── Sessions ─────────────────────────────────────────────────────────────────

int64_t Database::OpenSession(const std::string& app_name, const std::string& window_title) {
    const char* sql = "INSERT INTO sessions (app_name, window_title, started_at) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, app_name.c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, window_title.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, NowISO().c_str(),      -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(m_pDb);
}

void Database::CloseSession(int64_t id) {
    std::string now = NowISO();
    const char* sql =
        "UPDATE sessions "
        "SET ended_at = ?, "
        "    duration_s = CAST((julianday(?) - julianday(started_at)) * 86400 AS INTEGER) "
        "WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, now.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::optional<Session> Database::GetActiveSession() const {
    const char* sql =
        "SELECT id, app_name, window_title, category, started_at "
        "FROM sessions WHERE ended_at IS NULL ORDER BY started_at DESC LIMIT 1;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return std::nullopt;

    std::optional<Session> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        Session s;
        s.m_Id           = sqlite3_column_int64(stmt, 0);
        s.m_AppName      = ColText(stmt, 1);
        s.m_WindowTitle  = ColText(stmt, 2);
        s.m_Category     = ColText(stmt, 3);
        s.m_StartedAt    = ColText(stmt, 4);
        result = s;
    }
    sqlite3_finalize(stmt);
    return result;
}

std::vector<Session> Database::GetSessions(const std::string& from_iso, const std::string& to_iso) const {
    const char* sql =
        "SELECT id, app_name, window_title, category, started_at, ended_at, duration_s "
        "FROM sessions "
        "WHERE started_at >= ? AND started_at <= ? "
        "ORDER BY started_at;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};
    sqlite3_bind_text(stmt, 1, from_iso.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, to_iso.c_str(),   -1, SQLITE_TRANSIENT);

    std::vector<Session> sessions;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Session s;
        s.m_Id          = sqlite3_column_int64(stmt, 0);
        s.m_AppName     = ColText(stmt, 1);
        s.m_WindowTitle = ColText(stmt, 2);
        s.m_Category    = ColText(stmt, 3);
        s.m_StartedAt   = ColText(stmt, 4);
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) s.m_EndedAt   = ColText(stmt, 5);
        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) s.m_DurationS = sqlite3_column_int64(stmt, 6);
        sessions.push_back(std::move(s));
    }
    sqlite3_finalize(stmt);
    return sessions;
}

void Database::TagSession(int64_t id, const std::string& category) {
    const char* sql = "UPDATE sessions SET category = ? WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt,  1, category.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ── Rules ─────────────────────────────────────────────────────────────────────

int64_t Database::AddRule(const Rule& rule) {
    const char* sql =
        "INSERT INTO rules (pattern, match_field, category, priority) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, rule.m_Pattern.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, rule.m_MatchField.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, rule.m_Category.c_str(),   -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,  4, rule.m_Priority);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(m_pDb);
}

std::vector<Rule> Database::GetRules() const {
    const char* sql =
        "SELECT id, pattern, match_field, category, priority FROM rules ORDER BY priority DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    std::vector<Rule> rules;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Rule r;
        r.m_Id         = sqlite3_column_int64(stmt, 0);
        r.m_Pattern    = ColText(stmt, 1);
        r.m_MatchField = ColText(stmt, 2);
        r.m_Category   = ColText(stmt, 3);
        r.m_Priority   = sqlite3_column_int(stmt, 4);
        rules.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rules;
}

void Database::RemoveRule(int64_t id) {
    const char* sql = "DELETE FROM rules WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_int64(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// ── Categories ────────────────────────────────────────────────────────────────

int64_t Database::AddCategory(const std::string& name, const std::string& color) {
    const char* sql = "INSERT OR IGNORE INTO categories (name, color) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, name.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, color.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return sqlite3_last_insert_rowid(m_pDb);
}

std::vector<Category> Database::GetCategories() const {
    const char* sql = "SELECT id, name, color FROM categories ORDER BY name;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_pDb, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    std::vector<Category> cats;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Category c;
        c.m_Id   = sqlite3_column_int64(stmt, 0);
        c.m_Name = ColText(stmt, 1);
        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) c.m_Color = ColText(stmt, 2);
        cats.push_back(std::move(c));
    }
    sqlite3_finalize(stmt);
    return cats;
}

} // namespace DesktopWatcher
