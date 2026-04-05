#include "core/config.hpp"
#include <toml++/toml.hpp>
#include <cstdlib>
#include <filesystem>

namespace DesktopWatcher {

std::string DefaultConfigPath() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    std::string base = xdg
        ? std::string(xdg)
        : (std::string(std::getenv("HOME")) + "/.config");
    return base + "/desktop-watcher/config.toml";
}

std::string DefaultDbPath() {
    const char* xdg = std::getenv("XDG_DATA_HOME");
    std::string base = xdg
        ? std::string(xdg)
        : (std::string(std::getenv("HOME")) + "/.local/share");
    return base + "/desktop-watcher/desktop-watcher.db";
}

std::string Config::DbPath() const {
    if (!m_Daemon.m_DbPathOverride.empty())
        return m_Daemon.m_DbPathOverride;
    return DefaultDbPath();
}

Config LoadConfig(const std::string& path) {
    std::string cfg_path = path.empty() ? DefaultConfigPath() : path;
    Config cfg;

    if (!std::filesystem::exists(cfg_path))
        return cfg;

    toml::table tbl = toml::parse_file(cfg_path);

    if (auto* daemon = tbl["daemon"].as_table()) {
        if (auto v = (*daemon)["poll_interval_ms"].value<int>()) cfg.m_Daemon.m_PollIntervalMs = *v;
        if (auto v = (*daemon)["idle_threshold_s"].value<int>())  cfg.m_Daemon.m_IdleThresholdS  = *v;
        if (auto v = (*daemon)["db_path"].value<std::string>())   cfg.m_Daemon.m_DbPathOverride   = *v;
    }

    if (auto* display = tbl["display"].as_table()) {
        if (auto v = (*display)["time_format"].value<std::string>()) cfg.m_Display.m_TimeFormat  = *v;
        if (auto v = (*display)["week_starts"].value<std::string>()) cfg.m_Display.m_WeekStarts  = *v;
    }

    if (auto* rules_tbl = tbl["rules"].as_table()) {
        if (auto* entries = (*rules_tbl)["entry"].as_array()) {
            for (auto& entry : *entries) {
                if (auto* t = entry.as_table()) {
                    Rule r;
                    if (auto v = (*t)["pattern"].value<std::string>())    r.m_Pattern    = *v;
                    if (auto v = (*t)["match_field"].value<std::string>()) r.m_MatchField = *v;
                    if (auto v = (*t)["category"].value<std::string>())   r.m_Category   = *v;
                    if (auto v = (*t)["priority"].value<int>())           r.m_Priority   = *v;
                    if (!r.m_Pattern.empty() && !r.m_Category.empty())
                        cfg.m_Rules.push_back(std::move(r));
                }
            }
        }
    }

    return cfg;
}

} // namespace DesktopWatcher
