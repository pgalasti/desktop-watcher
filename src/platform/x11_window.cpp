#include "platform/window_info.hpp"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/extensions/scrnsaver.h>
#include <fmt/format.h>
#include <stdexcept>
#include <string>

namespace DesktopWatcher::Platform {

class X11WindowProvider : public IWindowProvider {
public:
    X11WindowProvider() {
        m_pDisplay = XOpenDisplay(nullptr);
        if (!m_pDisplay)
            throw std::runtime_error("Cannot open X display — is DISPLAY set?");

        int event_base, error_base;
        m_HasScreenSaver = XScreenSaverQueryExtension(m_pDisplay, &event_base, &error_base);
        if (!m_HasScreenSaver)
            fmt::print(stderr, "[desktop-watcher] warning: MIT-SCREEN-SAVER extension unavailable — idle detection disabled\n");
    }

    ~X11WindowProvider() override {
        if (m_pDisplay) XCloseDisplay(m_pDisplay);
    }

    WindowInfo GetActiveWindow() const override {
        WindowInfo info;
        Window root = DefaultRootWindow(m_pDisplay);

        Atom net_active = XInternAtom(m_pDisplay, "_NET_ACTIVE_WINDOW", True);
        if (net_active == None)
            return info;

        Atom   actual_type;
        int    actual_format;
        unsigned long nitems, bytes_after;
        unsigned char* prop = nullptr;

        if (XGetWindowProperty(m_pDisplay, root, net_active, 0, 1, False,
                               XA_WINDOW, &actual_type, &actual_format,
                               &nitems, &bytes_after, &prop) != Success || !prop)
            return info;

        Window active = *reinterpret_cast<Window*>(prop);
        XFree(prop);

        if (!active)
            return info;

        info.m_WindowTitle = GetWindowTitle(active);

        XClassHint hint;
        if (XGetClassHint(m_pDisplay, active, &hint)) {
            if (hint.res_name)  { info.m_AppName = hint.res_name;  XFree(hint.res_name);  }
            if (hint.res_class) { XFree(hint.res_class); }
        }

        Atom net_pid = XInternAtom(m_pDisplay, "_NET_WM_PID", True);
        if (net_pid != None) {
            prop = nullptr;
            if (XGetWindowProperty(m_pDisplay, active, net_pid, 0, 1, False,
                                   XA_CARDINAL, &actual_type, &actual_format,
                                   &nitems, &bytes_after, &prop) == Success && prop) {
                info.m_Pid = static_cast<int>(*reinterpret_cast<unsigned long*>(prop));
                XFree(prop);
            }
        }

        return info;
    }

    int64_t GetIdleTimeMs() const override {
        if (!m_HasScreenSaver) return 0;
        XScreenSaverInfo* ssi = XScreenSaverAllocInfo();
        if (!ssi) return 0;
        XScreenSaverQueryInfo(m_pDisplay, DefaultRootWindow(m_pDisplay), ssi);
        int64_t idle = static_cast<int64_t>(ssi->idle);
        XFree(ssi);
        return idle;
    }

private:
    Display* m_pDisplay{nullptr};
    bool     m_HasScreenSaver{false};

    std::string GetWindowTitle(Window w) const {
        Atom net_name = XInternAtom(m_pDisplay, "_NET_WM_NAME",  True);
        Atom utf8     = XInternAtom(m_pDisplay, "UTF8_STRING",   True);

        Atom   actual_type;
        int    actual_format;
        unsigned long nitems, bytes_after;
        unsigned char* prop = nullptr;

        if (net_name != None && utf8 != None) {
            if (XGetWindowProperty(m_pDisplay, w, net_name, 0, 1024, False,
                                   utf8, &actual_type, &actual_format,
                                   &nitems, &bytes_after, &prop) == Success && prop) {
                std::string title(reinterpret_cast<const char*>(prop));
                XFree(prop);
                return title;
            }
        }

        // Fallback to legacy WM_NAME
        char* name = nullptr;
        if (XFetchName(m_pDisplay, w, &name) && name) {
            std::string title(name);
            XFree(name);
            return title;
        }

        return "";
    }
};

std::unique_ptr<IWindowProvider> CreateX11WindowProvider() {
    return std::make_unique<X11WindowProvider>();
}

} // namespace DesktopWatcher::Platform
