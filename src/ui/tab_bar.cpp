#include "tab_bar.h"
#include <algorithm>

namespace nexora::ui {

TabBar::TabBar() {}

int64_t TabBar::CreateTab(const std::string& title, const std::string& url) {
    int64_t id = next_tab_id_++;
    auto tab = std::make_unique<TabItem>(id, title, url);
    tabs_.push_back(std::move(tab));
    SelectTab(id);
    return id;
}

bool TabBar::CloseTab(int64_t tab_id) {
    auto it = std::find_if(tabs_.begin(), tabs_.end(), [tab_id](const std::unique_ptr<TabItem>& t) {
        return t->GetId() == tab_id;
    });

    if (it == tabs_.end()) return false;

    size_t closed_idx = std::distance(tabs_.begin(), it);
    bool was_active = ((*it)->GetId() == active_tab_id_);

    tabs_.erase(it);

    if (tabs_.empty()) {
        active_tab_id_ = -1;
    } else if (was_active) {
        size_t next_idx = (closed_idx < tabs_.size()) ? closed_idx : (tabs_.size() - 1);
        SelectTab(tabs_[next_idx]->GetId());
    }

    if (close_cb_) close_cb_(tab_id);
    return true;
}

bool TabBar::SelectTab(int64_t tab_id) {
    bool found = false;
    for (auto& tab : tabs_) {
        if (tab->GetId() == tab_id) {
            tab->SetActive(true);
            active_tab_id_ = tab_id;
            found = true;
        } else {
            tab->SetActive(false);
        }
    }

    if (found && select_cb_) {
        select_cb_(tab_id);
    }
    return found;
}

bool TabBar::SelectTabByIndex(size_t index) {
    if (index >= tabs_.size()) return false;
    return SelectTab(tabs_[index]->GetId());
}

TabItem* TabBar::GetActiveTab() {
    return GetTabById(active_tab_id_);
}

const TabItem* TabBar::GetActiveTab() const {
    return GetTabById(active_tab_id_);
}

int TabBar::GetActiveTabIndex() const {
    for (size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i]->GetId() == active_tab_id_) return static_cast<int>(i);
    }
    return -1;
}

TabItem* TabBar::GetTabById(int64_t tab_id) {
    for (auto& t : tabs_) {
        if (t->GetId() == tab_id) return t.get();
    }
    return nullptr;
}

const TabItem* TabBar::GetTabById(int64_t tab_id) const {
    for (const auto& t : tabs_) {
        if (t->GetId() == tab_id) return t.get();
    }
    return nullptr;
}

TabItem* TabBar::GetTabByIndex(size_t index) {
    if (index < tabs_.size()) return tabs_[index].get();
    return nullptr;
}

const TabItem* TabBar::GetTabByIndex(size_t index) const {
    if (index < tabs_.size()) return tabs_[index].get();
    return nullptr;
}

void TabBar::MoveTab(size_t from_idx, size_t to_idx) {
    if (from_idx >= tabs_.size() || to_idx >= tabs_.size() || from_idx == to_idx) return;
    auto tab = std::move(tabs_[from_idx]);
    tabs_.erase(tabs_.begin() + from_idx);
    tabs_.insert(tabs_.begin() + to_idx, std::move(tab));
    if (reorder_cb_) reorder_cb_(static_cast<int>(from_idx), static_cast<int>(to_idx));
}

void TabBar::PinTab(int64_t tab_id, bool pinned) {
    auto tab = GetTabById(tab_id);
    if (!tab) return;
    tab->SetPinned(pinned);

    // Reorder pinned tabs to front
    std::stable_sort(tabs_.begin(), tabs_.end(), [](const std::unique_ptr<TabItem>& a, const std::unique_ptr<TabItem>& b) {
        return a->IsPinned() && !b->IsPinned();
    });
}

int TabBar::CreateGroup(const std::string& title, const std::string& color_hex) {
    int id = next_group_id_++;
    groups_.emplace_back(id, title, color_hex);
    return id;
}

void TabBar::AddTabToGroup(int64_t tab_id, int group_id) {
    auto tab = GetTabById(tab_id);
    if (tab) {
        tab->SetGroupId(group_id);
        auto grp = GetGroup(group_id);
        if (grp) grp->AddTab(tab_id);
    }
}

void TabBar::RemoveTabFromGroup(int64_t tab_id) {
    auto tab = GetTabById(tab_id);
    if (tab && tab->GetGroupId() > 0) {
        auto grp = GetGroup(tab->GetGroupId());
        if (grp) grp->RemoveTab(tab_id);
        tab->SetGroupId(-1);
    }
}

TabGroup* TabBar::GetGroup(int group_id) {
    for (auto& g : groups_) {
        if (g.GetId() == group_id) return &g;
    }
    return nullptr;
}

void TabBar::UpdateLayout(int bar_x, int bar_y, int bar_width, int bar_height) {
    bar_bounds_ = Rect{bar_x, bar_y, bar_width, bar_height};
    if (tabs_.empty()) {
        new_tab_btn_bounds_ = Rect{bar_x + 8, bar_y + (bar_height - 28) / 2, 28, 28};
        return;
    }

    int pin_width = 38;
    int max_tab_width = 240;
    int min_tab_width = 80;
    int new_tab_btn_size = 28;
    int margin_left = 8;
    int margin_right = 16;

    size_t pinned_count = 0;
    for (const auto& t : tabs_) {
        if (t->IsPinned()) pinned_count++;
    }
    size_t regular_count = tabs_.size() - pinned_count;

    int avail_width = bar_width - margin_left - margin_right - new_tab_btn_size - (static_cast<int>(pinned_count) * pin_width);
    int regular_width = max_tab_width;
    if (regular_count > 0) {
        regular_width = avail_width / static_cast<int>(regular_count);
        regular_width = std::max(min_tab_width, std::min(max_tab_width, regular_width));
    }

    int cur_x = bar_x + margin_left;
    for (auto& tab : tabs_) {
        int w = tab->IsPinned() ? pin_width : regular_width;
        tab->SetBounds(Rect{cur_x, bar_y + 4, w, bar_height - 4});
        cur_x += w + 2;
    }

    new_tab_btn_bounds_ = Rect{cur_x + 4, bar_y + (bar_height - new_tab_btn_size) / 2, new_tab_btn_size, new_tab_btn_size};
}

int TabBar::HitTestTab(int px, int py) const {
    for (size_t i = 0; i < tabs_.size(); ++i) {
        if (tabs_[i]->GetBounds().Contains(px, py)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TabBar::HitTestCloseButton(int px, int py, int64_t& out_tab_id) const {
    for (const auto& tab : tabs_) {
        if (tab->GetCloseButtonBounds().Contains(px, py)) {
            out_tab_id = tab->GetId();
            return true;
        }
    }
    return false;
}

bool TabBar::HitTestAudioButton(int px, int py, int64_t& out_tab_id) const {
    for (const auto& tab : tabs_) {
        if (tab->GetAudioButtonBounds().Contains(px, py)) {
            out_tab_id = tab->GetId();
            return true;
        }
    }
    return false;
}

bool TabBar::HitTestNewTabButton(int px, int py) const {
    return new_tab_btn_bounds_.Contains(px, py);
}

void TabBar::StartDrag(int64_t tab_id, int start_x, int start_y) {
    is_dragging_ = true;
    dragged_tab_id_ = tab_id;
    drag_start_x_ = start_x;
    drag_start_y_ = start_y;
}

void TabBar::UpdateDrag(int cur_x, int cur_y) {
    if (!is_dragging_) return;

    // Check if dragged outside tab strip (detach threshold)
    if (cur_y < (bar_bounds_.y - 20) || cur_y > (bar_bounds_.y + bar_bounds_.height + 30)) {
        if (detach_cb_) {
            detach_cb_(dragged_tab_id_, cur_x, cur_y);
            is_dragging_ = false;
            return;
        }
    }

    // Reorder tabs within strip
    int target_idx = HitTestTab(cur_x, cur_y);
    if (target_idx >= 0 && target_idx < static_cast<int>(tabs_.size())) {
        int cur_idx = -1;
        for (size_t i = 0; i < tabs_.size(); ++i) {
            if (tabs_[i]->GetId() == dragged_tab_id_) {
                cur_idx = static_cast<int>(i);
                break;
            }
        }
        if (cur_idx >= 0 && cur_idx != target_idx) {
            MoveTab(cur_idx, target_idx);
        }
    }
}

void TabBar::EndDrag() {
    is_dragging_ = false;
    dragged_tab_id_ = -1;
}

} // namespace nexora::ui
