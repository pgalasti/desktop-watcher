#include "daemon/poller.hpp"
#include <chrono>
#include <thread>
#include <fmt/format.h>

namespace DesktopWatcher::Daemon {

Poller::Poller(Config cfg, Database& db, Platform::IWindowProvider& provider)
    : m_Config(std::move(cfg)), m_Db(db), m_Provider(provider) {
    // Seed categorizer: DB rules first, then config rules on top
    m_Categorizer.LoadRules(m_Db.GetRules());
    for (const auto& r : m_Config.m_Rules)
        m_Categorizer.AddRule(r);
}

void Poller::Run() {
    m_Running = true;

    // Clean up any session left open by a previous unclean exit
    if (auto prev = m_Db.GetActiveSession())
        m_Db.CloseSession(prev->m_Id);

    const int64_t idle_threshold_ms =
        static_cast<int64_t>(m_Config.m_Daemon.m_IdleThresholdS) * 1000;
    const int     poll_ms = m_Config.m_Daemon.m_PollIntervalMs;

    bool was_idle = false;

    while (m_Running) {
        const int64_t idle_ms = m_Provider.GetIdleTimeMs();
        const bool    is_idle = (idle_ms >= idle_threshold_ms);

        if (is_idle) {
            if (!was_idle) {
                CloseActiveSession();
                fmt::print("[desktop-watcher] idle ({}s) — tracking paused\n", idle_ms / 1000);
                was_idle = true;
            }
        } else {
            if (was_idle) {
                fmt::print("[desktop-watcher] resumed tracking\n");
                was_idle = false;
            }

            WindowInfo current = m_Provider.GetActiveWindow();
            if (current.m_AppName    != m_LastWindow.m_AppName ||
                current.m_WindowTitle != m_LastWindow.m_WindowTitle) {
                HandleWindowChange(current);
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(poll_ms));
    }

    CloseActiveSession();
}

void Poller::Stop() {
    m_Running = false;
}

void Poller::HandleWindowChange(const WindowInfo& info) {
    CloseActiveSession();
    m_LastWindow = info;

    if (info.m_AppName.empty())
        return;

    m_ActiveSessionId = m_Db.OpenSession(info.m_AppName, info.m_WindowTitle);
    std::string category = m_Categorizer.Categorize(info);
    m_Db.TagSession(m_ActiveSessionId, category);

    fmt::print("[desktop-watcher] {} — {} [{}]\n",
               info.m_AppName, info.m_WindowTitle, category);
}

void Poller::CloseActiveSession() {
    if (m_ActiveSessionId != -1) {
        m_Db.CloseSession(m_ActiveSessionId);
        m_ActiveSessionId = -1;
    }
}

} // namespace DesktopWatcher::Daemon
