#include "test_framework.h"
#include "../src/ui/tab_bar.h"
#include "../src/ui/address_bar.h"
#include "../src/ui/navigation_controls.h"
#include "../src/ui/bookmark_bar.h"
#include "../src/ui/context_menu.h"
#include "../src/ui/theme_manager.h"
#include "../src/ui/browser_window.h"

using namespace nexora::ui;

TEST_CASE(BrowserUI, TabBarOperationsAndPinning) {
    TabBar tab_bar;
    int64_t t1 = tab_bar.CreateTab("Google", "https://google.com");
    int64_t t2 = tab_bar.CreateTab("GitHub", "https://github.com");
    int64_t t3 = tab_bar.CreateTab("Wikipedia", "https://wikipedia.org");

    ASSERT_EQ(tab_bar.GetTabCount(), 3);
    ASSERT_EQ(tab_bar.GetActiveTab()->GetId(), t3);

    ASSERT_TRUE(tab_bar.SelectTab(t1));
    ASSERT_EQ(tab_bar.GetActiveTab()->GetId(), t1);

    tab_bar.PinTab(t2, true);
    ASSERT_TRUE(tab_bar.GetTabById(t2)->IsPinned());
    // Pinned tab should move to index 0
    ASSERT_EQ(tab_bar.GetTabByIndex(0)->GetId(), t2);

    tab_bar.UpdateLayout(0, 0, 1200, 40);
    ASSERT_TRUE(tab_bar.GetTabByIndex(0)->GetBounds().width < tab_bar.GetTabByIndex(1)->GetBounds().width);

    ASSERT_TRUE(tab_bar.CloseTab(t3));
    ASSERT_EQ(tab_bar.GetTabCount(), 2);
}

TEST_CASE(BrowserUI, TabBarGroupingAndHitTesting) {
    TabBar tab_bar;
    int64_t t1 = tab_bar.CreateTab("Google", "https://google.com");
    int64_t t2 = tab_bar.CreateTab("Search 2", "https://bing.com");

    int grp_id = tab_bar.CreateGroup("Search", "#1a73e8");
    tab_bar.AddTabToGroup(t1, grp_id);
    tab_bar.AddTabToGroup(t2, grp_id);

    ASSERT_EQ(tab_bar.GetTabById(t1)->GetGroupId(), grp_id);
    ASSERT_EQ(tab_bar.GetGroup(grp_id)->GetTabs().size(), 2);

    tab_bar.UpdateLayout(0, 0, 1000, 40);
    int hit_idx = tab_bar.HitTestTab(50, 20);
    ASSERT_EQ(hit_idx, 0);

    ASSERT_TRUE(tab_bar.HitTestNewTabButton(tab_bar.GetNewTabButtonBounds().x + 5, tab_bar.GetNewTabButtonBounds().y + 5));
}

TEST_CASE(BrowserUI, AddressBarUrlNormalizationAndSecurity) {
    AddressBar addr_bar;
    std::string navigated_url;
    addr_bar.SetCallbacks([&](const std::string& url) { navigated_url = url; }, nullptr, nullptr);

    addr_bar.OnSubmit("github.com");
    ASSERT_EQ(navigated_url, "https://github.com");
    ASSERT_EQ(addr_bar.GetSecurityState(), SecurityState::Secure);

    addr_bar.OnSubmit("how does chromium rendering engine work");
    ASSERT_TRUE(navigated_url.find("google.com/search?q=") != std::string::npos);

    addr_bar.OnSubmit("nexora://taskmanager");
    ASSERT_EQ(navigated_url, "nexora://taskmanager");
    ASSERT_EQ(addr_bar.GetSecurityState(), SecurityState::Internal);

    addr_bar.SetBounds(Rect{100, 40, 600, 34});
    ASSERT_TRUE(addr_bar.GetSecurityIconBounds().x >= 100);
    ASSERT_TRUE(addr_bar.GetBookmarkStarBounds().x > 100);
    ASSERT_TRUE(addr_bar.GetProtectionShieldBounds().x > 100);
}

TEST_CASE(BrowserUI, ContextMenuCreationAndExecution) {
    bool closed = false;
    bool frozen = false;
    auto menu = ContextMenu::CreateTabContextMenu(10, false, false, false, [&](int64_t id, const std::string& action) {
        if (action == "close") closed = true;
        if (action == "freeze") frozen = true;
    });

    ASSERT_FALSE(menu.GetItems().empty());
    menu.ExecuteItem(9); // Close Tab
    ASSERT_TRUE(closed);

    menu.ExecuteItem(6); // Freeze Tab
    ASSERT_TRUE(frozen);
}

TEST_CASE(BrowserUI, ThemeManagerSwitching) {
    ThemeManager::Instance().SetDarkMode(true);
    ASSERT_TRUE(ThemeManager::Instance().IsDarkMode());
    auto dark_colors = ThemeManager::Instance().GetColors();
    ASSERT_EQ(dark_colors.background, 0x202124);

    ThemeManager::Instance().SetDarkMode(false);
    ASSERT_FALSE(ThemeManager::Instance().IsDarkMode());
    auto light_colors = ThemeManager::Instance().GetColors();
    ASSERT_EQ(light_colors.background, 0xF1F3F4);

    ThemeManager::Instance().SetDarkMode(true);
}

TEST_CASE(BrowserUI, BrowserWindowLayoutAndShortcuts) {
    BrowserWindow win(1);
    win.Create(0, 0, 1200, 800);

    ASSERT_EQ(win.GetTabBar().GetTabCount(), 1);
    ASSERT_EQ(win.GetTabBarBounds().height, 40);
    ASSERT_EQ(win.GetToolBarBounds().height, 42);
    ASSERT_TRUE(win.GetWebContentsBounds().height > 600);

    // Test Ctrl+T shortcut (New Tab)
    win.HandleKeyDown('T', true, false, false);
    ASSERT_EQ(win.GetTabBar().GetTabCount(), 2);

    // Navigate second tab to a website
    win.NavigateCurrentTab("https://github.com/wwater479/NEXORA");

    // Test Ctrl+W shortcut (Close Tab)
    win.HandleKeyDown('W', true, false, false);
    ASSERT_EQ(win.GetTabBar().GetTabCount(), 1);

    // Test Ctrl+Shift+T shortcut (Reopen Closed Tab)
    win.HandleKeyDown('T', true, true, false);
    ASSERT_EQ(win.GetTabBar().GetTabCount(), 2);
    ASSERT_EQ(win.GetTabBar().GetActiveTab()->GetUrl(), "https://github.com/wwater479/NEXORA");

    // Test Alt+Left & Alt+Right shortcuts
    win.HandleKeyDown(0x25, false, false, true); // Alt+Left (Back)
    win.HandleKeyDown(0x27, false, false, true); // Alt+Right (Forward)

    // Test Ctrl+Shift+R (Hard Reload)
    win.HandleKeyDown('R', true, true, false);
}
