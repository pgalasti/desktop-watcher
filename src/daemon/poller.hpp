#ifndef DW_POLLER_HPP
#define DW_POLLER_HPP

#include <atomic>
#include <cstdint>
#include "core/config.hpp"
#include "core/db.hpp"
#include "core/categorizer.hpp"
#include "platform/window_info.hpp"

namespace DesktopWatcher::Daemon {

class Poller {
public:
    Poller(Config cfg, Database& db, Platform::IWindowProvider& provider);

    void Run();   // Blocks until Stop() is called
    void Stop();  // Thread-safe

private:
    Config                     m_Config;
    Database&                  m_Db;
    Platform::IWindowProvider& m_Provider;
    Categorizer                m_Categorizer;

    std::atomic<bool> m_Running{false};
    int64_t           m_ActiveSessionId{-1};
    WindowInfo        m_LastWindow;

    void HandleWindowChange(const WindowInfo& info);
    void CloseActiveSession();
};

} // namespace DesktopWatcher::Daemon

#endif // DW_POLLER_HPP
