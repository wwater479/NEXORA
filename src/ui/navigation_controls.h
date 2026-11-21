#pragma once

#include "tab_item.h"
#include <functional>

namespace nexora::ui {

class NavigationControls {
public:
    using ActionCallback = std::function<void()>;

    NavigationControls();
    ~NavigationControls() = default;

    void SetCanGoBack(bool can_back) { can_go_back_ = can_back; }
    bool CanGoBack() const { return can_go_back_; }

    void SetCanGoForward(bool can_fwd) { can_go_forward_ = can_fwd; }
    bool CanGoForward() const { return can_go_forward_; }

    void SetIsLoading(bool loading) { is_loading_ = loading; }
    bool IsLoading() const { return is_loading_; }

    void UpdateLayout(int start_x, int start_y, int height);

    Rect GetBackBounds() const { return back_bounds_; }
    Rect GetForwardBounds() const { return forward_bounds_; }
    Rect GetReloadBounds() const { return reload_bounds_; }
    Rect GetHomeBounds() const { return home_bounds_; }
    Rect GetMenuBounds() const { return menu_bounds_; }
    Rect GetProfileBounds() const { return profile_bounds_; }

    int GetTotalWidth() const { return total_width_; }

    void SetCallbacks(ActionCallback back_cb, ActionCallback fwd_cb, ActionCallback reload_cb, ActionCallback home_cb, ActionCallback menu_cb) {
        back_cb_ = back_cb;
        fwd_cb_ = fwd_cb;
        reload_cb_ = reload_cb;
        home_cb_ = home_cb;
        menu_cb_ = menu_cb;
    }

    void OnBackClick() { if (can_go_back_ && back_cb_) back_cb_(); }
    void OnForwardClick() { if (can_go_forward_ && fwd_cb_) fwd_cb_(); }
    void OnReloadClick() { if (reload_cb_) reload_cb_(); }
    void OnHomeClick() { if (home_cb_) home_cb_(); }
    void OnMenuClick() { if (menu_cb_) menu_cb_(); }

private:
    bool can_go_back_ = false;
    bool can_go_forward_ = false;
    bool is_loading_ = false;

    Rect back_bounds_;
    Rect forward_bounds_;
    Rect reload_bounds_;
    Rect home_bounds_;
    Rect menu_bounds_;
    Rect profile_bounds_;
    int total_width_ = 140;

    ActionCallback back_cb_;
    ActionCallback fwd_cb_;
    ActionCallback reload_cb_;
    ActionCallback home_cb_;
    ActionCallback menu_cb_;
};

} // namespace nexora::ui
