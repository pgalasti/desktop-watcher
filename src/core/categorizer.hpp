#ifndef DW_CATEGORIZER_HPP
#define DW_CATEGORIZER_HPP

#include <vector>
#include <regex>
#include "models.hpp"

namespace DesktopWatcher {

class Categorizer {
public:
    void LoadRules(const std::vector<Rule>& rules);
    void AddRule(const Rule& rule);

    std::string Categorize(const WindowInfo& info) const;

private:
    struct CompiledRule {
        Rule        m_Rule;
        std::regex  m_Pattern;
    };

    std::vector<CompiledRule> m_Rules; // kept sorted by priority descending
};

} // namespace DesktopWatcher

#endif // DW_CATEGORIZER_HPP
