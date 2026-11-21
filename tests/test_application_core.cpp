#include "test_framework.h"
#include "../src/core/config_manager.h"
#include "../src/core/profile_manager.h"
#include "../src/core/update_manager.h"
#include "../src/core/window_manager.h"
#include "../src/util/path_util.h"

using namespace nexora::core;
using namespace nexora::util;

TEST_CASE(ApplicationCore, ConfigManagerDefaultAndPersistence) {
    std::string test_cfg_path = "./test_config.json";
    PathUtil::DeleteFile(test_cfg_path);

    ASSERT_TRUE(ConfigManager::Instance().Init(test_cfg_path));
    auto cfg = ConfigManager::Instance().GetConfig();
    ASSERT_EQ(cfg.search_engine_name, "Google");

    ConfigManager::Instance().SetTheme(ThemeMode::Dark);
    ASSERT_EQ(ConfigManager::Instance().GetTheme(), ThemeMode::Dark);

    ConfigManager::Instance().AddProtectedDomain("slack.com");
    ASSERT_TRUE(ConfigManager::Instance().IsDomainProtected("slack.com"));
    ASSERT_TRUE(ConfigManager::Instance().IsDomainProtected("app.slack.com"));
    ASSERT_FALSE(ConfigManager::Instance().IsDomainProtected("randomsite.org"));

    ConfigManager::Instance().RemoveProtectedDomain("slack.com");
    ASSERT_FALSE(ConfigManager::Instance().IsDomainProtected("slack.com"));

    PathUtil::DeleteFile(test_cfg_path);
}

TEST_CASE(ApplicationCore, ProfileManagerOperations) {
    std::string test_user_data = "./test_user_data";
    ProfileManager::Instance().Init(test_user_data);

    auto profiles = ProfileManager::Instance().GetProfiles();
    ASSERT_FALSE(profiles.empty());
    ASSERT_EQ(profiles[0].id, "Default");

    auto new_prof = ProfileManager::Instance().CreateProfile("Work Profile", "avatar_orange.png");
    ASSERT_EQ(new_prof.name, "Work Profile");

    ASSERT_TRUE(ProfileManager::Instance().SwitchProfile(new_prof.id));
    ASSERT_EQ(ProfileManager::Instance().GetActiveProfile().id, new_prof.id);

    auto incognito = ProfileManager::Instance().CreateIncognitoProfile();
    ASSERT_TRUE(incognito.is_incognito);

    ASSERT_TRUE(ProfileManager::Instance().DeleteProfile(new_prof.id));
    ASSERT_EQ(ProfileManager::Instance().GetActiveProfile().id, "Default");
}

TEST_CASE(ApplicationCore, UpdateManagerVersionInfo) {
    auto v = UpdateManager::Instance().GetVersionInfo();
    ASSERT_EQ(v.browser_name, "NEXORA");
    ASSERT_FALSE(v.browser_version.empty());
    ASSERT_FALSE(v.chromium_version.empty());
    ASSERT_FALSE(v.cef_version.empty());

    UpdateManager::Instance().SetChannel(UpdateChannel::Beta);
    ASSERT_EQ(UpdateManager::Instance().GetChannel(), UpdateChannel::Beta);

    bool callback_called = false;
    UpdateManager::Instance().CheckForUpdates([&](bool avail, const std::string& ver, const std::string& notes) {
        (void)avail; (void)ver; (void)notes;
        callback_called = true;
    });
    ASSERT_TRUE(callback_called);
}

TEST_CASE(ApplicationCore, WindowManagerMultiWindowAndTabTransfer) {
    WindowManager::Instance().CloseAllWindows();
    ASSERT_EQ(WindowManager::Instance().GetWindowCount(), 0);

    auto win1 = WindowManager::Instance().CreateWindow(50, 50, 1000, 700);
    ASSERT_NE(win1, nullptr);
    ASSERT_EQ(WindowManager::Instance().GetWindowCount(), 1);

    auto win2 = WindowManager::Instance().CreateWindow(100, 100, 1000, 700);
    ASSERT_NE(win2, nullptr);
    ASSERT_EQ(WindowManager::Instance().GetWindowCount(), 2);

    int64_t tab_id = win1->GetTabBar().CreateTab("GitHub", "https://github.com");
    ASSERT_NE(win1->GetTabBar().GetTabById(tab_id), nullptr);

    ASSERT_TRUE(WindowManager::Instance().TransferTab(win1->GetWindowId(), win2->GetWindowId(), tab_id));
    ASSERT_EQ(win1->GetTabBar().GetTabById(tab_id), nullptr);

    auto detached_win = WindowManager::Instance().DetachTabToNewWindow(win2->GetWindowId(), win2->GetTabBar().GetActiveTab()->GetId(), 200, 200);
    ASSERT_NE(detached_win, nullptr);
    ASSERT_EQ(WindowManager::Instance().GetWindowCount(), 3);

    auto session = WindowManager::Instance().CaptureCurrentSession();
    ASSERT_EQ(session.windows.size(), 3);

    WindowManager::Instance().CloseAllWindows();
    ASSERT_EQ(WindowManager::Instance().GetWindowCount(), 0);
}
