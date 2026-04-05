#include <csignal>
#include <stdexcept>
#include <fmt/format.h>
#include "core/config.hpp"
#include "core/db.hpp"
#include "daemon/poller.hpp"
#include "platform/window_info.hpp"

using namespace DesktopWatcher;

// Global pointer for signal handler access — set once in main before Run().
static Daemon::Poller* g_pPoller = nullptr;

static void OnSignal(int sig) {
    fmt::print("\n[desktop-watcher] received signal {}, shutting down...\n", sig);
    if (g_pPoller) g_pPoller->Stop();
}

int main() {
    std::signal(SIGINT,  OnSignal);
    std::signal(SIGTERM, OnSignal);

    try {
        Config   cfg = LoadConfig();
        Database db(cfg.DbPath());
        db.InitSchema();

        auto provider = Platform::CreateWindowProvider();
        Daemon::Poller poller(cfg, db, *provider);
        g_pPoller = &poller;

        fmt::print("[desktop-watcher] starting — poll {}ms  idle threshold {}s  db {}\n",
                   cfg.m_Daemon.m_PollIntervalMs,
                   cfg.m_Daemon.m_IdleThresholdS,
                   cfg.DbPath());

        poller.Run();
        fmt::print("[desktop-watcher] stopped.\n");

    } catch (const std::exception& ex) {
        fmt::print(stderr, "[desktop-watcher] fatal: {}\n", ex.what());
        return 1;
    }

    return 0;
}
