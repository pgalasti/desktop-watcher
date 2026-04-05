#include <CLI/CLI.hpp>
#include <fmt/format.h>
#include <stdexcept>
#include "core/config.hpp"
#include "core/db.hpp"

using namespace DesktopWatcher;


static Database OpenDb() {
    Config cfg = LoadConfig();
    Database db(cfg.DbPath());
    db.InitSchema();
    return db;
}


int main(int argc, char** argv) {
    CLI::App app{"DesktopWatcher — personal productivity tracker"};
    app.require_subcommand(0, 1);
    app.set_help_flag("-h,--help", "Show help");

    auto* cmd_status = app.add_subcommand("status", "Show what's currently being tracked");
    cmd_status->callback([&]() {
        try {
            Database db = OpenDb();
            if (auto s = db.GetActiveSession()) {
                fmt::print("Tracking:  {} — {}\n", s->m_AppName, s->m_WindowTitle);
                fmt::print("Category:  {}\n",  s->m_Category);
                fmt::print("Since:     {}\n",  s->m_StartedAt);
            } else {
                fmt::print("Not currently tracking (is desktop-watcherd running?).\n");
            }
        } catch (const std::exception& ex) {
            fmt::print(stderr, "error: {}\n", ex.what());
        }
    });

    app.add_subcommand("today",      "Summary of today's usage")
       ->callback([]() { fmt::print("'today' not yet implemented\n"); });

    app.add_subcommand("report",     "Detailed usage report")
       ->callback([]() { fmt::print("'report' not yet implemented\n"); });

    app.add_subcommand("top",        "Top apps by time spent")
       ->callback([]() { fmt::print("'top' not yet implemented\n"); });

    app.add_subcommand("categories", "List configured categories")
       ->callback([]() { fmt::print("'categories' not yet implemented\n"); });

    CLI11_PARSE(app, argc, argv);

    if (app.get_subcommands().empty())
        fmt::print("{}", app.help());

    return 0;
}
