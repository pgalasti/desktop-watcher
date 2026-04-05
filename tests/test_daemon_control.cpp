#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>

#include <unistd.h>

#include "daemon/daemon_control.hpp"

using namespace DesktopWatcher::Daemon;

// Pin XDG_RUNTIME_DIR to /tmp so tests never touch the real PID file.
static void UseTestDir() {
    setenv("XDG_RUNTIME_DIR", "/tmp", 1);
}

static void Cleanup() {
    std::filesystem::remove(PidFilePath());
}

// ── PidFilePath ───────────────────────────────────────────────────────────────

static void TestPidFilePathUsesXdgRuntimeDir() {
    setenv("XDG_RUNTIME_DIR", "/tmp/dw_test_runtime", 1);
    std::string path = PidFilePath();
    assert(path.rfind("/tmp/dw_test_runtime/", 0) == 0);
    assert(path.find("desktop-watcherd.pid") != std::string::npos);
    setenv("XDG_RUNTIME_DIR", "/tmp", 1);  // restore
    std::cout << "TestPidFilePathUsesXdgRuntimeDir: PASS\n";
}

// ── WritePidFile / ReadPidFile ────────────────────────────────────────────────

static void TestWriteReadRoundTrip() {
    UseTestDir();
    Cleanup();

    WritePidFile();
    int64_t pid = ReadPidFile();
    assert(pid == static_cast<int64_t>(getpid()));

    Cleanup();
    std::cout << "TestWriteReadRoundTrip: PASS\n";
}

static void TestReadMissingReturnsNegativeOne() {
    UseTestDir();
    Cleanup();  // ensure file is absent

    assert(ReadPidFile() == -1);
    std::cout << "TestReadMissingReturnsNegativeOne: PASS\n";
}

static void TestWriteOverwritesPreviousValue() {
    UseTestDir();
    Cleanup();

    // Write a dummy PID directly, then overwrite with the real one
    {
        std::ofstream f(PidFilePath());
        f << "99999\n";
    }
    WritePidFile();
    assert(ReadPidFile() == static_cast<int64_t>(getpid()));

    Cleanup();
    std::cout << "TestWriteOverwritesPreviousValue: PASS\n";
}

// ── RemovePidFile ─────────────────────────────────────────────────────────────

static void TestRemoveDeletesFile() {
    UseTestDir();
    WritePidFile();
    assert(std::filesystem::exists(PidFilePath()));

    RemovePidFile();
    assert(!std::filesystem::exists(PidFilePath()));
    std::cout << "TestRemoveDeletesFile: PASS\n";
}

static void TestRemoveIsNoopWhenMissing() {
    UseTestDir();
    Cleanup();

    RemovePidFile();  // must not throw
    assert(!std::filesystem::exists(PidFilePath()));
    std::cout << "TestRemoveIsNoopWhenMissing: PASS\n";
}

// ── IsProcessAlive ────────────────────────────────────────────────────────────

static void TestCurrentProcessIsAlive() {
    assert(IsProcessAlive(static_cast<int64_t>(getpid())));
    std::cout << "TestCurrentProcessIsAlive: PASS\n";
}

static void TestPid1IsAlive() {
    // PID 1 (init / systemd) is always present on Linux
    assert(IsProcessAlive(1));
    std::cout << "TestPid1IsAlive: PASS\n";
}

static void TestInvalidPidsAreNotAlive() {
    assert(!IsProcessAlive(-1));
    assert(!IsProcessAlive(0));
    // INT_MAX is astronomically unlikely to be a real PID
    assert(!IsProcessAlive(2147483647LL));
    std::cout << "TestInvalidPidsAreNotAlive: PASS\n";
}

static void TestStalePidFileIsDetectedViaIsProcessAlive() {
    UseTestDir();
    Cleanup();

    // Write a PID that cannot be a live process (INT_MAX)
    {
        std::ofstream f(PidFilePath());
        f << "2147483647\n";
    }
    int64_t pid = ReadPidFile();
    assert(pid == 2147483647LL);
    assert(!IsProcessAlive(pid));  // stale

    Cleanup();
    std::cout << "TestStalePidFileIsDetectedViaIsProcessAlive: PASS\n";
}

// ─────────────────────────────────────────────────────────────────────────────

int main() {
    TestPidFilePathUsesXdgRuntimeDir();
    TestWriteReadRoundTrip();
    TestReadMissingReturnsNegativeOne();
    TestWriteOverwritesPreviousValue();
    TestRemoveDeletesFile();
    TestRemoveIsNoopWhenMissing();
    TestCurrentProcessIsAlive();
    TestPid1IsAlive();
    TestInvalidPidsAreNotAlive();
    TestStalePidFileIsDetectedViaIsProcessAlive();
    std::cout << "All DaemonControl tests passed.\n";
    return 0;
}
