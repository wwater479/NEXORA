#pragma once

#include "tab_item.h"
#include "tab_group.h"
#include <vector>
#include <memory>
#include <functional>

namespace nexora::ui {

class TabBar {
public:
    using TabSelectCallback = std::function<void(int64_t tab_id)>;
    using TabCloseCallback = std::function<void(int64_t tab_id)>;
    using TabNewCallback = std::function<void()>;
    using TabReorderCallback = std::function<void(int from_idx, int to_idx)>;
    using TabDetachCallback = std::function<void(int64_t tab_id, int screen_x, int screen_y)>;

    TabBar();
    ~TabBar() = default;

    int64_t CreateTab(const std::string& title = "New Tab", const std::string& url = "nexora://newtab");
    bool CloseTab(int64_t tab_id);
    bool SelectTab(int64_t tab_id);
    bool SelectTabByIndex(size_t index);

    TabItem* GetActiveTab();
    const TabItem* GetActiveTab() const;
    int GetActiveTabIndex() const;

    TabItem* GetTabById(int64_t tab_id);
    const TabItem* GetTabById(int64_t tab_id) const;

    TabItem* GetTabByIndex(size_t index);
    const TabItem* GetTabByIndex(size_t index) const;

    size_t GetTabCount() const { return tabs_.size(); }
    const std::vector<std::unique_ptr<TabItem>>& GetTabs() const { return tabs_; }

    void MoveTab(size_t from_idx, size_t to_idx);
    void PinTab(int64_t tab_id, bool pinned);

    // Tab Groups
    int CreateGroup(const std::string& title, const std::string& color_hex = "#1a73e8");
    void AddTabToGroup(int64_t tab_id, int group_id);
    void RemoveTabFromGroup(int64_t tab_id);
    TabGroup* GetGroup(int group_id);

    // Layout
    void UpdateLayout(int bar_x, int bar_y, int bar_width, int bar_height);
    Rect GetNewTabButtonBounds() const { return new_tab_btn_bounds_; }

    // Hit Testing
    int HitTestTab(int px, int py) const;
    bool HitTestCloseButton(int px, int py, int64_t& out_tab_id) const;
    bool HitTestAudioButton(int px, int py, int64_t& out_tab_id) const;
    bool HitTestNewTabButton(int px, int py) const;

    // Dragging
    void StartDrag(int64_t tab_id, int start_x, int start_y);
    void UpdateDrag(int cur_x, int cur_y);
    void EndDrag();
    bool IsDragging() const { return is_dragging_; }
    int64_t GetDraggedTabId() const { return dragged_tab_id_; }

    void SetCallbacks(TabSelectCallback select_cb, TabCloseCallback close_cb, TabNewCallback new_cb,
                      TabReorderCallback reorder_cb, TabDetachCallback detach_cb) {
        select_cb_ = select_cb;
        close_cb_ = close_cb;
        new_cb_ = new_cb;
        reorder_cb_ = reorder_cb;
        detach_cb_ = detach_cb;
    }

private:
    std::vector<std::unique_ptr<TabItem>> tabs_;
    std::vector<TabGroup> groups_;
    int64_t active_tab_id_ = -1;
    int64_t next_tab_id_ = 1;
    int next_group_id_ = 1;

    Rect bar_bounds_;
    Rect new_tab_btn_bounds_;

    bool is_dragging_ = false;
    int64_t dragged_tab_id_ = -1;
    int drag_start_x_ = 0;
    int drag_start_y_ = 0;

    TabSelectCallback select_cb_;
    TabCloseCallback close_cb_;
    TabNewCallback new_cb_;
    TabReorderCallback reorder_cb_;
    TabDetachCallback detach_cb_;
};

} // namespace nexora::ui
