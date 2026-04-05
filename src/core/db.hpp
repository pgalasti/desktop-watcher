#ifndef DW_DB_HPP
#define DW_DB_HPP

#include <string>
#include <vector>
#include <optional>
#include <sqlite3.h>
#include "models.hpp"

namespace DesktopWatcher {

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    Database(const Database&)            = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    void InitSchema();

    // Sessions
    int64_t                OpenSession(const std::string& app_name, const std::string& window_title);
    void                   CloseSession(int64_t id);
    std::optional<Session> GetActiveSession() const;
    std::vector<Session>   GetSessions(const std::string& from_iso, const std::string& to_iso) const;
    void                   TagSession(int64_t id, const std::string& category);

    // Rules
    int64_t            AddRule(const Rule& rule);
    std::vector<Rule>  GetRules() const;
    void               RemoveRule(int64_t id);

    // Categories
    int64_t                 AddCategory(const std::string& name, const std::string& color = "");
    std::vector<Category>   GetCategories() const;

private:
    sqlite3* m_pDb{nullptr};

    void        Exec(const std::string& sql);
    std::string NowISO() const;
    static std::string ColText(sqlite3_stmt* stmt, int col);
};

} // namespace DesktopWatcher

#endif // DW_DB_HPP
