#ifndef DW_MODELS_HPP
#define DW_MODELS_HPP

#include <string>
#include <optional>
#include <cstdint>

namespace DesktopWatcher {

struct Session {
    int64_t     m_Id{};
    std::string m_AppName;
    std::string m_WindowTitle;
    std::string m_Category{"uncategorized"};
    std::string m_StartedAt;
    std::optional<std::string> m_EndedAt;
    std::optional<int64_t>     m_DurationS;
};

struct Category {
    int64_t     m_Id{};
    std::string m_Name;
    std::optional<std::string> m_Color;
};

struct Rule {
    int64_t     m_Id{};
    std::string m_Pattern;
    std::string m_MatchField{"app_name"};  // "app_name" | "window_title"
    std::string m_Category;
    int         m_Priority{0};
};

struct WindowInfo {
    std::string m_AppName;
    std::string m_WindowTitle;
    int         m_Pid{-1};
};

} // namespace DesktopWatcher

#endif // DW_MODELS_HPP
