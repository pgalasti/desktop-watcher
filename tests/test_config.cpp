#include <cassert>
#include <fstream>
#include <filesystem>
#include <iostream>
#include "core/config.hpp"

using namespace DesktopWatcher;

static void TestDefaults() {
    // Loading a non-existent path should return defaults without throwing
    Config cfg = LoadConfig("/tmp/dw_nonexistent.toml");
    assert(cfg.m_Daemon.m_PollIntervalMs == 1000);
    assert(cfg.m_Daemon.m_IdleThresholdS == 300);
    assert(cfg.m_Display.m_TimeFormat    == "24h");
    assert(cfg.m_Display.m_WeekStarts    == "monday");
    assert(cfg.m_Rules.empty());
    std::cout << "TestDefaults: PASS\n";
}

static void TestParsing() {
    const std::string path = "/tmp/dw_test_config.toml";
    {
        std::ofstream f(path);
        f << "[daemon]\n";
        f << "poll_interval_ms = 500\n";
        f << "idle_threshold_s = 60\n";
        f << "[display]\n";
        f << "time_format = \"12h\"\n";
        f << "[rules]\n";
        f << "[[rules.entry]]\n";
        f << "pattern    = \"firefox\"\n";
        f << "match_field = \"app_name\"\n";
        f << "category   = \"browsing\"\n";
        f << "priority   = 10\n";
    }

    Config cfg = LoadConfig(path);
    assert(cfg.m_Daemon.m_PollIntervalMs == 500);
    assert(cfg.m_Daemon.m_IdleThresholdS == 60);
    assert(cfg.m_Display.m_TimeFormat    == "12h");
    assert(cfg.m_Rules.size()            == 1);
    assert(cfg.m_Rules[0].m_Pattern      == "firefox");
    assert(cfg.m_Rules[0].m_Category     == "browsing");
    assert(cfg.m_Rules[0].m_Priority     == 10);

    std::filesystem::remove(path);
    std::cout << "TestParsing: PASS\n";
}

int main() {
    TestDefaults();
    TestParsing();
    std::cout << "All Config tests passed.\n";
    return 0;
}
