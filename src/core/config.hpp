#ifndef DW_CONFIG_HPP
#define DW_CONFIG_HPP

#include <string>
#include <vector>
#include "models.hpp"

namespace DesktopWatcher {

struct DaemonConfig {
    int         m_PollIntervalMs{1000};
    int         m_IdleThresholdS{300};
    std::string m_DbPathOverride;
};

struct DisplayConfig {
    std::string m_TimeFormat{"24h"};
    std::string m_WeekStarts{"monday"};
};

struct Config {
    DaemonConfig        m_Daemon;
    DisplayConfig       m_Display;
    std::vector<Rule>   m_Rules;

    std::string DbPath() const;
};

Config      LoadConfig(const std::string& path = "");
std::string DefaultConfigPath();
std::string DefaultDbPath();

} // namespace DesktopWatcher

#endif // DW_CONFIG_HPP
