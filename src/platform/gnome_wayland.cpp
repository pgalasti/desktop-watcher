#include "platform/window_info.hpp"
#include <atspi/atspi.h>
#include <gio/gio.h>
#include <fmt/format.h>
#include <stdexcept>
#include <string>

namespace DesktopWatcher::Platform {

class GnomeWaylandWindowProvider : public IWindowProvider {
public:
    GnomeWaylandWindowProvider() {
        if (atspi_init() < 0)
            throw std::runtime_error("Failed to initialise AT-SPI2");

        GError* err = nullptr;
        m_pBus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &err);
        if (err) {
            std::string msg = err->message;
            g_error_free(err);
            atspi_exit();
            throw std::runtime_error("D-Bus session bus unavailable: " + msg);
        }
    }

    ~GnomeWaylandWindowProvider() override {
        if (m_pBus) g_object_unref(m_pBus);
        atspi_exit();
    }

    WindowInfo GetActiveWindow() const override {
        WindowInfo info;

        int n_desktops = atspi_get_desktop_count();
        for (int d = 0; d < n_desktops; d++) {
            AtspiAccessible* desktop = atspi_get_desktop(d);
            if (!desktop) continue;

            int n_apps = atspi_accessible_get_child_count(desktop, nullptr);
            for (int a = 0; a < n_apps; a++) {
                AtspiAccessible* app = atspi_accessible_get_child_at_index(desktop, a, nullptr);
                if (!app) continue;

                int n_wins = atspi_accessible_get_child_count(app, nullptr);
                for (int w = 0; w < n_wins; w++) {
                    AtspiAccessible* win = atspi_accessible_get_child_at_index(app, w, nullptr);
                    if (!win) { continue; }

                    AtspiStateSet* states = atspi_accessible_get_state_set(win);
                    bool active = states && atspi_state_set_contains(states, ATSPI_STATE_ACTIVE);
                    if (states) g_object_unref(states);

                    if (active) {
                        gchar* app_name  = atspi_accessible_get_name(app, nullptr);
                        gchar* win_title = atspi_accessible_get_name(win, nullptr);
                        if (app_name)  { info.m_AppName     = app_name;  g_free(app_name);  }
                        if (win_title) { info.m_WindowTitle = win_title; g_free(win_title); }
                        g_object_unref(win);
                        g_object_unref(app);
                        g_object_unref(desktop);
                        return info;
                    }
                    g_object_unref(win);
                }
                g_object_unref(app);
            }
            g_object_unref(desktop);
        }
        return info;
    }

    int64_t GetIdleTimeMs() const override {
        GError* err = nullptr;
        GVariant* result = g_dbus_connection_call_sync(
            m_pBus,
            "org.gnome.Mutter.IdleMonitor",
            "/org/gnome/Mutter/IdleMonitor/Core",
            "org.gnome.Mutter.IdleMonitor",
            "GetIdletime",
            nullptr,
            G_VARIANT_TYPE("(t)"),
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            &err
        );

        if (err) {
            g_error_free(err);
            return 0;
        }

        guint64 idle_ms = 0;
        g_variant_get(result, "(t)", &idle_ms);
        g_variant_unref(result);
        return static_cast<int64_t>(idle_ms);
    }

private:
    GDBusConnection* m_pBus{nullptr};
};

std::unique_ptr<IWindowProvider> CreateX11WindowProvider();

std::unique_ptr<IWindowProvider> CreateWindowProvider() {
    const char* session = std::getenv("XDG_SESSION_TYPE");
    if (session && std::string(session) == "wayland") {
        return std::make_unique<GnomeWaylandWindowProvider>();
    }
    return CreateX11WindowProvider();
}

} // namespace DesktopWatcher::Platform
