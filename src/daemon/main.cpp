#include <chrono>
#include <csignal>
#include <stdexcept>
#include <thread>

#include <fmt/format.h>
#include <CLI/CLI.hpp>
#include <unistd.h>

#include "core/config.hpp"
#include "core/db.hpp"
#include "daemon/daemon_control.hpp"
#include "daemon/poller.hpp"
#include "platform/window_info.hpp"

using namespace DesktopWatcher;
using namespace DesktopWatcher::Daemon;

// Global pointer for signal handler access — set once before Run().
static Poller* g_pPoller = nullptr;

static void OnSignal(int /*sig*/) {
    if (g_pPoller) g_pPoller->Stop();
}


static int CmdStart(bool foreground) {

    // Bail out if already running
    int64_t existing = ReadPidFile();
    if (existing > 0 && IsProcessAlive(existing)) {
        fmt::print(stderr, "[dwd] already running (PID {})\n", existing);
        return 1;
    }

    if (!foreground) {
        fmt::print("[dwd] starting in background...\n");
        Daemonize();
        // Grandchild continues here; stdin/stdout/stderr -> /dev/null
    }

    std::signal(SIGINT,  OnSignal);
    std::signal(SIGTERM, OnSignal);

    WritePidFile();
    int exit_code = 0;

    try {
        Config   cfg = LoadConfig();
        Database db(cfg.DbPath());
        db.InitSchema();

        auto provider = Platform::CreateWindowProvider();
        Poller poller(cfg, db, *provider);
        g_pPoller = &poller;

        poller.Run();

    } catch (const std::exception& ex) {
        fmt::print(stderr, "[dwd] fatal: {}\n", ex.what());
        exit_code = 1;
    }

    RemovePidFile();
    return exit_code;
}

static int CmdStop() {
    int64_t pid = ReadPidFile();
    if (pid <= 0 || !IsProcessAlive(pid)) {
        fmt::print("[dwd] not running\n");
        return 0;
    }

    ::kill(static_cast<pid_t>(pid), SIGTERM);
    fmt::print("[dwd] sent SIGTERM to PID {}\n", pid);

    // Poll up to 5 s for the process to exit
    for (int i = 0; i < 50; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!IsProcessAlive(pid)) {
            fmt::print("[dwd] stopped.\n");
            return 0;
        }
    }

    fmt::print(stderr, "[dwd] warning: daemon still running after 5 s\n");
    return 1;
}

static int CmdStatus() {
    int64_t pid = ReadPidFile();
    if (pid <= 0 || !IsProcessAlive(pid)) {
        fmt::print("[dwd] not running\n");
        return 1;
    }
    fmt::print("[dwd] running (PID {})\n", pid);
    return 0;
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char** argv) {
    CLI::App app{"DesktopWatcher daemon manager", "dwd"};
    app.require_subcommand(1);

    // start
    bool foreground = false;
    auto* start_cmd = app.add_subcommand("start", "Start the daemon");
    start_cmd->add_flag("-f,--foreground", foreground,
                        "Run in the foreground instead of daemonizing");

    // stop
    auto* stop_cmd = app.add_subcommand("stop", "Stop the running daemon");

    // status
    auto* status_cmd = app.add_subcommand("status", "Show whether the daemon is running");

    CLI11_PARSE(app, argc, argv);

    if (app.got_subcommand(start_cmd))  return CmdStart(foreground);
    if (app.got_subcommand(stop_cmd))   return CmdStop();
    if (app.got_subcommand(status_cmd)) return CmdStatus();

    return 0;
}
