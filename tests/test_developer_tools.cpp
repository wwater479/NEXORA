#include "test_framework.h"
#include "../src/devtools/task_manager.h"
#include "../src/devtools/memory_profiler.h"
#include "../src/devtools/cpu_monitor.h"
#include "../src/devtools/gpu_monitor.h"
#include "../src/devtools/devtools_manager.h"
#include "../src/memory/tab_lifecycle_manager.h"

using namespace nexora::devtools;
using namespace nexora::memory;

TEST_CASE(DeveloperTools, TaskManagerListingAndSorting) {
    TaskManager::Instance().StartMonitoring(1000);
    ASSERT_TRUE(TaskManager::Instance().IsMonitoring());

    TabLifecycleManager::Instance().RegisterTab(301, 1, "https://cnn.com", "CNN");
    TabLifecycleManager::Instance().RegisterTab(302, 1, "https://youtube.com", "YouTube");

    auto tasks_ram_desc = TaskManager::Instance().GetTasks(TaskSortColumn::PrivateBytes, false);
    ASSERT_TRUE(tasks_ram_desc.size() >= 3); // Browser, GPU, Network + tabs

    // Test toggle protection
    bool new_prot = TaskManager::Instance().ToggleTabProtection(301);
    ASSERT_TRUE(new_prot);
    auto tab_info = TabLifecycleManager::Instance().GetTabInfo(301);
    ASSERT_TRUE(tab_info.factors.is_user_protected);

    // Untoggle protection so discard works
    TaskManager::Instance().ToggleTabProtection(301);
    TabLifecycleManager::Instance().SetTabActive(301, false);

    // Force discard
    ASSERT_TRUE(TaskManager::Instance().ForceDiscardTab(301));
    ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(301), TabLifecycleState::Discarded);

    TabLifecycleManager::Instance().UnregisterTab(301);
    TabLifecycleManager::Instance().UnregisterTab(302);
    TaskManager::Instance().StopMonitoring();
}

TEST_CASE(DeveloperTools, MemoryProfilerSnapshotsAndExport) {
    MemoryProfiler::Instance().ClearHistory();
    ASSERT_EQ(MemoryProfiler::Instance().GetHistory().size(), 0);

    auto snap1 = MemoryProfiler::Instance().TakeSnapshot();
    ASSERT_TRUE(snap1.timestamp > 0);
    ASSERT_EQ(MemoryProfiler::Instance().GetHistory().size(), 1);

    std::string json_export = MemoryProfiler::Instance().ExportJson();
    ASSERT_FALSE(json_export.empty());
    ASSERT_TRUE(json_export.find("snapshots") != std::string::npos);
}

TEST_CASE(DeveloperTools, CpuAndGpuMonitors) {
    double cpu_pct = CpuMonitor::Instance().SampleBrowserCpuPercent();
    ASSERT_TRUE(cpu_pct >= 0.0 && cpu_pct <= 100.0);

    auto gpu_info = GpuMonitor::Instance().GetGpuInfo();
    ASSERT_TRUE(gpu_info.is_hardware_accelerated);
    ASSERT_EQ(gpu_info.crash_restart_count, 0);

    GpuMonitor::Instance().RecordGpuCrash();
    ASSERT_EQ(GpuMonitor::Instance().GetGpuInfo().crash_restart_count, 1);
}

TEST_CASE(DeveloperTools, DevToolsManagerEndpoints) {
    DevToolsManager::Instance().Init(true, 9222);
    ASSERT_TRUE(DevToolsManager::Instance().IsRemoteDebuggingEnabled());
    ASSERT_EQ(DevToolsManager::Instance().GetRemoteDebuggingPort(), 9222);
    ASSERT_EQ(DevToolsManager::Instance().GetRemoteDebuggingUrl(), "http://127.0.0.1:9222");

    DevToolsManager::Instance().OpenDevToolsForTab(401);
    ASSERT_TRUE(DevToolsManager::Instance().IsDevToolsOpenForTab(401));

    DevToolsManager::Instance().CloseDevToolsForTab(401);
    ASSERT_FALSE(DevToolsManager::Instance().IsDevToolsOpenForTab(401));
}
