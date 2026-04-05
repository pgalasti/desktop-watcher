#ifndef DW_IDLE_HPP
#define DW_IDLE_HPP

#include <cstdint>
#include "platform/window_info.hpp"

namespace DesktopWatcher::Daemon {

class IdleDetector {
public:
    IdleDetector(Platform::IWindowProvider& provider, int64_t threshold_ms);

    bool    IsIdle()    const;
    int64_t GetIdleMs() const;

private:
    Platform::IWindowProvider& m_Provider;
    int64_t                    m_ThresholdMs;
};

} // namespace DesktopWatcher::Daemon

#endif // DW_IDLE_HPP
