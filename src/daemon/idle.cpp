#include "daemon/idle.hpp"

namespace DesktopWatcher::Daemon {

IdleDetector::IdleDetector(Platform::IWindowProvider& provider, int64_t threshold_ms)
    : m_Provider(provider), m_ThresholdMs(threshold_ms) {}

bool IdleDetector::IsIdle() const {
    return m_Provider.GetIdleTimeMs() >= m_ThresholdMs;
}

int64_t IdleDetector::GetIdleMs() const {
    return m_Provider.GetIdleTimeMs();
}

} // namespace DesktopWatcher::Daemon
