#include "tab_group.h"
#include <algorithm>

namespace nexora::ui {

TabGroup::TabGroup(int group_id, const std::string& title, const std::string& color_hex)
    : group_id_(group_id), title_(title), color_hex_(color_hex) {}

void TabGroup::AddTab(int64_t tab_id) {
    if (!HasTab(tab_id)) {
        tab_ids_.push_back(tab_id);
    }
}

void TabGroup::RemoveTab(int64_t tab_id) {
    auto it = std::find(tab_ids_.begin(), tab_ids_.end(), tab_id);
    if (it != tab_ids_.end()) {
        tab_ids_.erase(it);
    }
}

bool TabGroup::HasTab(int64_t tab_id) const {
    return std::find(tab_ids_.begin(), tab_ids_.end(), tab_id) != tab_ids_.end();
}

} // namespace nexora::ui
