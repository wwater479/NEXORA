#include "test_framework.h"
#include "../src/memory/system_memory_monitor.h"
#include "../src/memory/memory_pressure_controller.h"
#include "../src/memory/tab_priority_manager.h"
#include "../src/memory/tab_lifecycle_manager.h"
#include "../src/memory/background_resource_manager.h"
#include "../src/memory/memory_accounting.h"

using namespace nexora::memory;

TEST_CASE(MemoryEngine, SystemMemoryMonitorAndSimulatedPressure) {
    SystemMemoryMonitor::Instance().Sample();
    auto sys_mem = SystemMemoryMonitor::Instance().GetSystemMemoryInfo();
    ASSERT_TRUE(sys_mem.total_physical_bytes > 0);

    // Test simulation
    SystemMemoryMonitor::Instance().SetSimulatedSystemMemory(16ULL * 1024 * 1024 * 1024, 1ULL * 1024 * 1024 * 1024, 94);
    ASSERT_EQ(SystemMemoryMonitor::Instance().GetMemoryLoadPercent(), 94);

    SystemMemoryMonitor::Instance().ClearSimulatedSystemMemory();
}

TEST_CASE(MemoryEngine, MemoryPressureControllerTransitions) {
    MemoryPressureController::Instance().Init(80, 90);

    // Normal pressure
    SystemMemoryMonitor::Instance().SetSimulatedSystemMemory(16ULL * 1024 * 1024 * 1024, 8ULL * 1024 * 1024 * 1024, 50);
    ASSERT_EQ(MemoryPressureController::Instance().EvaluatePressure(), MemoryPressureLevel::Normal);

    // Moderate pressure (85% load)
    SystemMemoryMonitor::Instance().SetSimulatedSystemMemory(16ULL * 1024 * 1024 * 1024, 2ULL * 1024 * 1024 * 1024, 85);
    ASSERT_EQ(MemoryPressureController::Instance().EvaluatePressure(), MemoryPressureLevel::Moderate);

    // Critical pressure (95% load)
    SystemMemoryMonitor::Instance().SetSimulatedSystemMemory(16ULL * 1024 * 1024 * 1024, 800ULL * 1024 * 1024, 95);
    ASSERT_EQ(MemoryPressureController::Instance().EvaluatePressure(), MemoryPressureLevel::Critical);

    SystemMemoryMonitor::Instance().ClearSimulatedSystemMemory();
}

TEST_CASE(MemoryEngine, TabPriorityManagerScoringAndImmunity) {
    TabPriorityFactors active_tab;
    active_tab.tab_id = 1;
    active_tab.is_active = true;
    ASSERT_TRUE(TabPriorityManager::Instance().IsTabImmuneFromDiscard(active_tab));
    ASSERT_EQ(TabPriorityManager::Instance().CalculatePriorityScore(active_tab), 1000.0);

    TabPriorityFactors audio_tab;
    audio_tab.tab_id = 2;
    audio_tab.is_active = false;
    audio_tab.is_audio_playing = true;
    ASSERT_TRUE(TabPriorityManager::Instance().IsTabImmuneFromDiscard(audio_tab));

    TabPriorityFactors protected_tab;
    protected_tab.tab_id = 3;
    protected_tab.is_active = false;
    protected_tab.is_user_protected = true;
    ASSERT_TRUE(TabPriorityManager::Instance().IsTabImmuneFromDiscard(protected_tab));

    TabPriorityFactors idle_tab1;
    idle_tab1.tab_id = 4;
    idle_tab1.is_active = false;
    idle_tab1.last_active_timestamp = 1000;

    TabPriorityFactors idle_tab2;
    idle_tab2.tab_id = 5;
    idle_tab2.is_active = false;
    idle_tab2.is_pinned = true;
    idle_tab2.last_active_timestamp = 1000;

    double score1 = TabPriorityManager::Instance().CalculatePriorityScore(idle_tab1);
    double score2 = TabPriorityManager::Instance().CalculatePriorityScore(idle_tab2);
    ASSERT_TRUE(score2 > score1); // Pinned tab must have higher score than unpinned idle tab

    std::vector<TabPriorityFactors> all_tabs = {active_tab, audio_tab, protected_tab, idle_tab2, idle_tab1};
    auto candidates = TabPriorityManager::Instance().GetDiscardCandidates(all_tabs);

    // Only un-immune tabs should be in candidates, ordered ascending by score
    ASSERT_EQ(candidates.size(), 2);
    ASSERT_EQ(candidates[0].tab_id, 4); // idle_tab1 has lowest score, first to discard
    ASSERT_EQ(candidates[1].tab_id, 5); // idle_tab2 is pinned, discarded after
}

TEST_CASE(MemoryEngine, FourStateTabLifecycleTransitionsAndRestoration) {
    TabLifecycleManager::Instance().Init(300, 900);

    int64_t t1 = 101;
    int64_t t2 = 102;
    int64_t t3 = 103;

    TabLifecycleManager::Instance().RegisterTab(t1, 1, "https://github.com", "GitHub");
    TabLifecycleManager::Instance().RegisterTab(t2, 1, "https://google.com", "Google");
    TabLifecycleManager::Instance().RegisterTab(t3, 1, "https://wikipedia.org", "Wikipedia");

    ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(t1), TabLifecycleState::Active);

    // Deactivate t1 (switch to t2)
    TabLifecycleManager::Instance().SetTabActive(t1, false);
    ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(t1), TabLifecycleState::Background);

    // Freeze t1
    ASSERT_TRUE(TabLifecycleManager::Instance().FreezeTab(t1));
    ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(t1), TabLifecycleState::Frozen);

    // Discard t1
    TabLifecycleManager::Instance().UpdateTabMemory(t1, 150 * 1024 * 1024); // 150MB
    ASSERT_TRUE(TabLifecycleManager::Instance().DiscardTab(t1));
    ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(t1), TabLifecycleState::Discarded);

    DiscardedTabState d_state;
    ASSERT_TRUE(TabLifecycleManager::Instance().GetDiscardedState(t1, d_state));
    ASSERT_EQ(d_state.url, "https://github.com");
    ASSERT_EQ(d_state.title, "GitHub");

    // Reactivate t1 (Restore)
    ASSERT_TRUE(TabLifecycleManager::Instance().RestoreTab(t1));
    ASSERT_EQ(TabLifecycleManager::Instance().GetTabState(t1), TabLifecycleState::Active);

    auto stats = TabLifecycleManager::Instance().GetMemoryStats();
    ASSERT_TRUE(stats.total_memory_saved_bytes > 0);

    TabLifecycleManager::Instance().UnregisterTab(t1);
    TabLifecycleManager::Instance().UnregisterTab(t2);
    TabLifecycleManager::Instance().UnregisterTab(t3);
}

TEST_CASE(MemoryEngine, BackgroundResourceManagerThrottling) {
    BackgroundResourceManager::Instance().SetTabThrottled(201, true);
    ASSERT_TRUE(BackgroundResourceManager::Instance().IsTabThrottled(201));
    ASSERT_FALSE(BackgroundResourceManager::Instance().IsTabThrottled(202));

    BackgroundResourceManager::Instance().RegisterAudioStream(201, true);
    ASSERT_TRUE(BackgroundResourceManager::Instance().IsAudioStreamPlaying(201));
}

TEST_CASE(MemoryEngine, FormInputAndMediaImmunityVerification) {
    TabPriorityFactors form_tab;
    form_tab.tab_id = 701;
    form_tab.is_active = false;
    form_tab.is_form_input_active = true;
    ASSERT_TRUE(TabPriorityManager::Instance().IsTabImmuneFromDiscard(form_tab));

    TabPriorityFactors dl_tab;
    dl_tab.tab_id = 702;
    dl_tab.is_active = false;
    dl_tab.has_active_download = true;
    ASSERT_TRUE(TabPriorityManager::Instance().IsTabImmuneFromDiscard(dl_tab));

    TabPriorityFactors call_tab;
    call_tab.tab_id = 703;
    call_tab.is_active = false;
    call_tab.is_video_call = true;
    ASSERT_TRUE(TabPriorityManager::Instance().IsTabImmuneFromDiscard(call_tab));
}

TEST_CASE(MemoryEngine, MemoryAccountingAccurateAccountingAndDelta) {
    auto report = MemoryAccounting::Instance().GenerateReport();
    ASSERT_TRUE(report.total_working_set_bytes > 0);
    ASSERT_TRUE(report.total_private_bytes > 0);
    ASSERT_TRUE(report.browser_process_count >= 1);
    ASSERT_TRUE(report.gpu_process_count >= 1);
    ASSERT_TRUE(report.utility_process_count >= 1);

    // Register a tab, measure discard delta
    int64_t test_tab = 777;
    TabLifecycleManager::Instance().RegisterTab(test_tab, 1, "https://heavy-site.com", "Heavy Site");
    TabLifecycleManager::Instance().SetTabActive(test_tab, false);
    TabLifecycleManager::Instance().UpdateTabMemory(test_tab, 120 * 1024 * 1024);

    uint64_t ws_freed = 0, priv_freed = 0;
    bool discarded = MemoryAccounting::Instance().MeasureDiscardDelta(test_tab, ws_freed, priv_freed);
    ASSERT_TRUE(discarded);
    ASSERT_TRUE(ws_freed > 0);
    ASSERT_TRUE(priv_freed > 0);

    TabLifecycleManager::Instance().UnregisterTab(test_tab);
}
