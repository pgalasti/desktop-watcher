#include "core/categorizer.hpp"
#include <algorithm>

namespace DesktopWatcher {

void Categorizer::LoadRules(const std::vector<Rule>& rules) {
    m_Rules.clear();
    for (const auto& r : rules)
        AddRule(r);
}

void Categorizer::AddRule(const Rule& rule) {
    try {
        CompiledRule cr;
        cr.m_Rule    = rule;
        cr.m_Pattern = std::regex(rule.m_Pattern,
                                  std::regex::icase | std::regex::ECMAScript);
        m_Rules.push_back(std::move(cr));
        std::stable_sort(m_Rules.begin(), m_Rules.end(),
            [](const CompiledRule& a, const CompiledRule& b) {
                return a.m_Rule.m_Priority > b.m_Rule.m_Priority;
            });
    } catch (const std::regex_error&) {
        // Skip rules with invalid regex patterns
    }
}

std::string Categorizer::Categorize(const WindowInfo& info) const {
    for (const auto& cr : m_Rules) {
        const std::string& field = (cr.m_Rule.m_MatchField == "window_title")
                                   ? info.m_WindowTitle
                                   : info.m_AppName;
        if (std::regex_search(field, cr.m_Pattern))
            return cr.m_Rule.m_Category;
    }
    return "uncategorized";
}

} // namespace DesktopWatcher
