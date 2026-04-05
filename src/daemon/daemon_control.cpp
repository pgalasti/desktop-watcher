#include "daemon/daemon_control.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fmt/format.h>

namespace DesktopWatcher::Daemon {

std::string PidFilePath() {
    const char* xdg = std::getenv("XDG_RUNTIME_DIR");
    std::string dir = xdg ? xdg : fmt::format("/run/user/{}", static_cast<unsigned>(getuid()));
    return dir + "/desktop-watcherd.pid";
}

int64_t ReadPidFile() {
    std::ifstream f(PidFilePath());
    if (!f.is_open()) return -1;
    int64_t pid = -1;
    f >> pid;
    return (f ? pid : -1);
}

void WritePidFile() {
    std::ofstream f(PidFilePath(), std::ios::trunc);
    if (!f.is_open())
        throw std::runtime_error("Cannot write PID file: " + PidFilePath());
    f << static_cast<long>(getpid()) << '\n';
}

void RemovePidFile() {
    std::error_code ec;
    std::filesystem::remove(PidFilePath(), ec);
    // Ignore errors (e.g. file already removed)
}

bool IsProcessAlive(int64_t pid) {
    if (pid <= 0) return false;
    int rc = ::kill(static_cast<pid_t>(pid), 0);
    // rc == 0  → process exists and we can signal it
    // EPERM   → process exists but we lack permission (still alive)
    // ESRCH   → no such process
    return rc == 0 || (rc == -1 && errno == EPERM);
}

void Daemonize() {
    pid_t pid = ::fork();
    if (pid < 0) throw std::runtime_error("fork() failed");
    if (pid > 0) ::_exit(0);  // parent exits immediately

    // Become session leader
    if (::setsid() < 0) throw std::runtime_error("setsid() failed");

    // Second fork — ensure we can never re-acquire a controlling terminal
    pid = ::fork();
    if (pid < 0) throw std::runtime_error("second fork() failed");
    if (pid > 0) ::_exit(0);  // session leader exits

    // Grandchild: we are now a proper daemon
    ::umask(0022);

    // Redirect standard file descriptors to /dev/null
    int devnull = ::open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        ::dup2(devnull, STDIN_FILENO);
        ::dup2(devnull, STDOUT_FILENO);
        ::dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) ::close(devnull);
    }
}

} // namespace DesktopWatcher::Daemon
