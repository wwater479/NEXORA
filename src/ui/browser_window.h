#pragma once

#include "tab_bar.h"
#include "address_bar.h"
#include "navigation_controls.h"
#include "bookmark_bar.h"
#include "download_ui.h"
#include "../engine/browser_instance.h"
#include "../util/win32_util.h"
#include <memory>
#include <string>
#include <map>

namespace nexora::ui {

class BrowserWindow {
public:
    explicit BrowserWindow(int window_id);
    ~BrowserWindow();

    bool Create(int x = 100, int y = 100, int width = 1200, int height = 800, bool maximized = false);
    void Destroy();
    void Show(bool show = true);

    int GetWindowId() const { return window_id_; }
    HWND GetHwnd() const { return hwnd_; }

    TabBar& GetTabBar() { return tab_bar_; }
    const TabBar& GetTabBar() const { return tab_bar_; }

    AddressBar& GetAddressBar() { return address_bar_; }
    const AddressBar& GetAddressBar() const { return address_bar_; }

    NavigationControls& GetNavigationControls() { return nav_controls_; }
    BookmarkBar& GetBookmarkBar() { return bookmark_bar_; }
    DownloadUI& GetDownloadUI() { return download_ui_; }

    std::shared_ptr<engine::BrowserInstance> GetActiveBrowser();
    std::shared_ptr<engine::BrowserInstance> GetBrowserForTab(int64_t tab_id);

    void UpdateLayout(int width, int height);

    Rect GetTabBarBounds() const { return tab_bar_bounds_; }
    Rect GetToolBarBounds() const { return toolbar_bounds_; }
    Rect GetBookmarkBarBounds() const { return bookmark_bar_bounds_; }
    Rect GetWebContentsBounds() const { return web_contents_bounds_; }

    void HandleKeyDown(int virtual_key, bool ctrl_pressed, bool shift_pressed, bool alt_pressed);

    void NavigateCurrentTab(const std::string& url);
    void ReloadCurrentTab();
    void StopCurrentTab();
    void GoBackCurrentTab();
    void GoForwardCurrentTab();

    bool IsFullscreen() const { return is_fullscreen_; }
    void ToggleFullscreen();

    // Factory Win32 Window Procedure
#if defined(_WIN32)
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
#endif

private:
    std::shared_ptr<engine::BrowserInstance> CreateBrowserForTab(int64_t tab_id, const std::string& url);
    void CloseBrowserForTab(int64_t tab_id);

    int window_id_;
    HWND hwnd_ = nullptr;
    bool is_fullscreen_ = false;

    TabBar tab_bar_;
    AddressBar address_bar_;
    NavigationControls nav_controls_;
    BookmarkBar bookmark_bar_;
    DownloadUI download_ui_;

    std::map<int64_t, std::shared_ptr<engine::BrowserInstance>> browsers_;

    Rect tab_bar_bounds_;
    Rect toolbar_bounds_;
    Rect bookmark_bar_bounds_;
    Rect web_contents_bounds_;
};

} // namespace nexora::ui
