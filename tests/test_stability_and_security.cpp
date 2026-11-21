#include "test_framework.h"
#include "../src/engine/cef_app_handler.h"
#include "../src/engine/browser_instance.h"
#include "../src/engine/process_manager.h"
#include "../src/memory/tab_lifecycle_manager.h"
#include "../src/memory/tab_priority_manager.h"
#include "../src/memory/memory_accounting.h"
#include "../src/memory/memory_pressure_controller.h"
#include "../src/devtools/gpu_monitor.h"
#include "../src/core/profile_manager.h"
#include "../src/core/window_manager.h"
#include "../src/storage/download_store.h"
#include "../src/util/path_util.h"
#include <vector>
#include <string>

using namespace nexora::engine;
using namespace nexora::memory;
using namespace nexora::core;
using namespace nexora::devtools;
using namespace nexora::storage;

TEST_CASE(SecurityAudit, SwitchValidationAndSandboxPreservation) {
    CefAppHandler app_handler;
    auto default_switches = app_handler.GetOptimizationSwitches();

    // Verify all default switches pass security audit
    std::vector<std::string> warnings;
    bool is_secure = CefAppHandler::ValidateSecurityConfiguration(default_switches, warnings);
    ASSERT_TRUE(is_secure);
    ASSERT_EQ(warnings.size(), 0);

    // Verify documented switches have proper sandboxing & site isolation flags
    auto doc_switches = app_handler.GetDocumentedSwitches();
    ASSERT_TRUE(doc_switches.size() >= 7);
    for (const auto& doc : doc_switches) {
        ASSERT_TRUE(doc.preserves_sandbox);
        ASSERT_TRUE(doc.preserves_site_isolation);
        ASSERT_FALSE(doc.justification.empty());
    }

    // Verify dangerous switches are strictly rejected
    std::vector<std::string> dangerous = {
        "--no-sandbox",
        "--disable-site-isolation-trials",
        "--single-process",
        "--in-process-gpu",
        "--disable-web-security"
    };

    for (const auto& bad_switch : dangerous) {
        std::vector<std::string> bad_warnings;
        bool result = CefAppHandler::ValidateSecurityConfiguration({bad_switch}, bad_warnings);
        ASSERT_FALSE(result);
        ASSERT_TRUE(bad_warnings.size() > 0);
    }
}

TEST_CASE(ProcessIsolation, CrossOriginSiteIsolationAndPids) {
    BrowserEventCallbacks cb;
    BrowserInstance site_a(801, 1, nullptr, cb);
    BrowserInstance site_b(802, 1, nullptr, cb);

    site_a.Create(nexora::ui::Rect{0, 0, 800, 600}, "https://bank.example.com/login");
    site_b.Create(nexora::ui::Rect{0, 0, 800, 600}, "https://untrusted-ads.example.org");

    // Assign isolated PIDs representing strict Site Isolation
    site_a.SetRendererPid(20001);
    site_b.SetRendererPid(20002);

    ProcessManager::Instance().RegisterProcess(20001, "renderer", 801);
    ProcessManager::Instance().RegisterProcess(20002, "renderer", 802);

    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(801), 20001);
    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(802), 20002);
    ASSERT_TRUE(ProcessManager::Instance().GetPidForTab(801) != ProcessManager::Instance().GetPidForTab(802));

    site_a.Destroy();
    site_b.Destroy();
}

TEST_CASE(ProcessIsolation, RendererProcessSharingAndSafeRefcounting) {
    uint32_t shared_pid = 30001;
    ProcessManager::Instance().RegisterProcess(shared_pid, "renderer", 811);
    ProcessManager::Instance().RegisterProcess(shared_pid, "renderer", 812);

    ASSERT_TRUE(ProcessManager::Instance().IsProcessShared(shared_pid));
    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(811), shared_pid);
    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(812), shared_pid);

    // Unregister tab 811: shared PID must NOT be terminated
    bool can_terminate_first = ProcessManager::Instance().UnregisterTabFromProcess(shared_pid, 811);
    ASSERT_FALSE(can_terminate_first); // Still used by tab 812!
    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(812), shared_pid);

    // Unregister tab 812: now refcount is 0, process can be safely terminated
    bool can_terminate_second = ProcessManager::Instance().UnregisterTabFromProcess(shared_pid, 812);
    ASSERT_TRUE(can_terminate_second);
    ASSERT_EQ(ProcessManager::Instance().GetPidForTab(812), 0);
}

TEST_CASE(Stability, RendererCrashResiliencyAndNoBrowserCrash) {
    BrowserEventCallbacks cb;
    bool error_reported = false;
    int reported_error_code = 0;

    cb.on_load_error = [&](int64_t tid, int err_code, const std::string& err_txt, const std::string& failed_url) {
        (void)tid; (void)err_txt; (void)failed_url;
        error_reported = true;
        reported_error_code = err_code;
    };

    BrowserInstance tab_inst(803, 1, nullptr, cb);
    tab_inst.Create(nexora::ui::Rect{0, 0, 800, 600}, "https://crash-test.com");
    tab_inst.SetRendererPid(20003);
    ProcessManager::Instance().RegisterProcess(20003, "renderer", 803);

    // Simulate renderer crash
    ProcessManager::Instance().HandleRendererCrash(20003, 803);
    auto all_proc = ProcessManager::Instance().GetAllProcesses();
    bool found_crashed = false;
    for (const auto& p : all_proc) {
        if (p.pid == 20003 && p.is_crashed) {
            found_crashed = true;
            break;
        }
    }
    ASSERT_TRUE(found_crashed);

    // Browser instance remains valid and can recover/reload
    tab_inst.Reload(true);
    ASSERT_EQ(tab_inst.GetCurrentUrl(), "https://crash-test.com");

    tab_inst.Destroy();
}

TEST_CASE(Stability, GpuCrashWatchdogAndRecovery) {
    int init_crashes = GpuMonitor::Instance().GetGpuInfo().crash_restart_count;
    ProcessManager::Instance().HandleGpuCrash();
    ASSERT_EQ(GpuMonitor::Instance().GetGpuInfo().crash_restart_count, init_crashes + 1);
    ASSERT_TRUE(GpuMonitor::Instance().GetGpuInfo().is_hardware_accelerated);
}

TEST_CASE(Stability, ThirtyTabsHighConcurrencyStressTest) {
    TabLifecycleManager::Instance().Init(100, 200);

    const int tab_count = 30;
    std::vector<int64_t> tab_ids;

    for (int i = 0; i < tab_count; ++i) {
        int64_t tid = 8000 + i;
        tab_ids.push_back(tid);
        TabLifecycleManager::Instance().RegisterTab(tid, 1, "https://example.org/item" + std::to_string(i), "Item " + std::to_string(i));
        TabLifecycleManager::Instance().UpdateTabMemory(tid, 60 * 1024 * 1024);
        if (i > 0) {
            TabLifecycleManager::Instance().SetTabActive(tid, false);
        }
    }

    ASSERT_EQ(TabLifecycleManager::Instance().GetAllTabs().size(), tab_count);

    // Rapid freeze and discard cycles across 30 tabs
    for (int i = 5; i < tab_count; ++i) {
        ASSERT_TRUE(TabLifecycleManager::Instance().DiscardTab(tab_ids[i]));
        ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(tab_ids[i]), TabLifecycleState::Discarded);
    }

    // Restore discarded tabs
    for (int i = 5; i < 15; ++i) {
        ASSERT_TRUE(TabLifecycleManager::Instance().RestoreTab(tab_ids[i]));
        ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(tab_ids[i]), TabLifecycleState::Active);
    }

    // Cleanup all 30 tabs
    for (int64_t tid : tab_ids) {
        TabLifecycleManager::Instance().UnregisterTab(tid);
    }

    ASSERT_EQ(TabLifecycleManager::Instance().GetAllTabs().size(), 0);
}

TEST_CASE(Stability, RapidTabCreationAndDeletionChurn) {
    const int cycles = 100;
    for (int i = 0; i < cycles; ++i) {
        int64_t tid = 9500 + i;
        TabLifecycleManager::Instance().RegisterTab(tid, 1, "https://churn.test/page", "Churn Page");
        TabLifecycleManager::Instance().SetTabActive(tid, false);
        TabLifecycleManager::Instance().FreezeTab(tid);
        TabLifecycleManager::Instance().DiscardTab(tid);
        TabLifecycleManager::Instance().UnregisterTab(tid);
    }

    ASSERT_EQ(TabLifecycleManager::Instance().GetAllTabs().size(), 0);
}

TEST_CASE(Stability, ProfileStoragePartitioning) {
    std::string root_dir = "./test_profiles_root";
    ProfileManager::Instance().Init(root_dir);

    auto p1 = ProfileManager::Instance().CreateProfile("Profile_Alpha");
    auto p2 = ProfileManager::Instance().CreateProfile("Profile_Beta");

    ASSERT_FALSE(p1.id.empty());
    ASSERT_FALSE(p2.id.empty());
    ASSERT_TRUE(p1.id != p2.id);
    ASSERT_TRUE(p1.profile_dir_path != p2.profile_dir_path);
}
