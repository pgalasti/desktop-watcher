#include <cassert>
#include <filesystem>
#include <iostream>
#include "core/db.hpp"

using namespace DesktopWatcher;

static void TestOpenSession() {
    const std::string path = "/tmp/dw_test.db";
    std::filesystem::remove(path);

    Database db(path);
    db.InitSchema();

    int64_t id = db.OpenSession("vim", "test.cpp - vim");
    assert(id > 0);

    auto active = db.GetActiveSession();
    assert(active.has_value());
    assert(active->m_AppName     == "vim");
    assert(active->m_WindowTitle == "test.cpp - vim");
    assert(!active->m_EndedAt.has_value());

    db.CloseSession(id);
    assert(!db.GetActiveSession().has_value());

    std::cout << "TestOpenSession: PASS\n";
    std::filesystem::remove(path);
}

static void TestTagSession() {
    const std::string path = "/tmp/dw_test_tag.db";
    std::filesystem::remove(path);

    Database db(path);
    db.InitSchema();

    int64_t id = db.OpenSession("steam", "Portal 2");
    db.TagSession(id, "gaming");
    db.CloseSession(id);

    auto sessions = db.GetSessions("2000-01-01T00:00:00Z", "2099-01-01T00:00:00Z");
    assert(!sessions.empty());
    assert(sessions.back().m_Category == "gaming");

    std::cout << "TestTagSession: PASS\n";
    std::filesystem::remove(path);
}

static void TestRules() {
    const std::string path = "/tmp/dw_test_rules.db";
    std::filesystem::remove(path);

    Database db(path);
    db.InitSchema();

    Rule r;
    r.m_Pattern    = "firefox";
    r.m_MatchField = "app_name";
    r.m_Category   = "browsing";
    r.m_Priority   = 10;

    int64_t rid = db.AddRule(r);
    assert(rid > 0);

    auto rules = db.GetRules();
    assert(rules.size() == 1);
    assert(rules[0].m_Category == "browsing");

    db.RemoveRule(rid);
    assert(db.GetRules().empty());

    std::cout << "TestRules: PASS\n";
    std::filesystem::remove(path);
}

int main() {
    TestOpenSession();
    TestTagSession();
    TestRules();
    std::cout << "All DB tests passed.\n";
    return 0;
}
