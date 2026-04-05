#include <cassert>
#include <iostream>
#include "core/categorizer.hpp"

using namespace DesktopWatcher;

static Rule MakeRule(const std::string& pattern, const std::string& field,
                     const std::string& category, int priority) {
    Rule r;
    r.m_Pattern    = pattern;
    r.m_MatchField = field;
    r.m_Category   = category;
    r.m_Priority   = priority;
    return r;
}

static void TestBasicMatch() {
    Categorizer c;
    c.AddRule(MakeRule("firefox|chromium", "app_name", "browsing", 10));
    c.AddRule(MakeRule("vim|nvim",         "app_name", "coding",   10));

    WindowInfo info;
    info.m_AppName = "firefox";
    assert(c.Categorize(info) == "browsing");

    info.m_AppName = "nvim";
    assert(c.Categorize(info) == "coding");

    info.m_AppName = "slack";
    assert(c.Categorize(info) == "uncategorized");

    std::cout << "TestBasicMatch: PASS\n";
}

static void TestWindowTitleMatch() {
    Categorizer c;
    c.AddRule(MakeRule("github\\.com", "window_title", "browsing", 5));

    WindowInfo info;
    info.m_AppName     = "firefox";
    info.m_WindowTitle = "anthropics/claude — GitHub - Firefox";
    assert(c.Categorize(info) == "browsing");

    info.m_WindowTitle = "Hacker News";
    assert(c.Categorize(info) == "uncategorized");

    std::cout << "TestWindowTitleMatch: PASS\n";
}

static void TestPriorityOrdering() {
    Categorizer c;
    // Lower priority rule added first
    c.AddRule(MakeRule("code",   "app_name", "work",   5));
    c.AddRule(MakeRule("code",   "app_name", "coding", 10));

    WindowInfo info;
    info.m_AppName = "code";
    // Higher priority "coding" rule should win
    assert(c.Categorize(info) == "coding");

    std::cout << "TestPriorityOrdering: PASS\n";
}

static void TestInvalidRegex() {
    Categorizer c;
    // Should not crash — invalid patterns are silently skipped
    c.AddRule(MakeRule("[invalid(", "app_name", "bad", 10));
    c.AddRule(MakeRule("firefox",   "app_name", "browsing", 5));

    WindowInfo info;
    info.m_AppName = "firefox";
    assert(c.Categorize(info) == "browsing");

    std::cout << "TestInvalidRegex: PASS\n";
}

int main() {
    TestBasicMatch();
    TestWindowTitleMatch();
    TestPriorityOrdering();
    TestInvalidRegex();
    std::cout << "All Categorizer tests passed.\n";
    return 0;
}
