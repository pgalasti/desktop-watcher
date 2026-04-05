#ifndef DW_WINDOW_INFO_HPP
#define DW_WINDOW_INFO_HPP

#include <memory>
#include "core/models.hpp"

namespace DesktopWatcher::Platform {

class IWindowProvider {
public:
    virtual ~IWindowProvider() = default;

    virtual WindowInfo GetActiveWindow() const = 0;

    virtual int64_t GetIdleTimeMs() const = 0;
};

std::unique_ptr<IWindowProvider> CreateWindowProvider();

} // namespace DesktopWatcher::Platform

#endif // DW_WINDOW_INFO_HPP
