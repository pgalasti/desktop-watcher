#ifndef DW_DAEMON_CONTROL_HPP
#define DW_DAEMON_CONTROL_HPP

#include <cstdint>
#include <string>

namespace DesktopWatcher::Daemon {

std::string PidFilePath();

int64_t ReadPidFile();

void WritePidFile();

void RemovePidFile();

bool IsProcessAlive(int64_t pid);

void Daemonize();

} // namespace DesktopWatcher::Daemon

#endif // DW_DAEMON_CONTROL_HPP
