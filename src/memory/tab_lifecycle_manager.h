#pragma once

#include "memory_types.h"
#include <map>
#include <vector>
#include <mutex>
#include <functional>
#include <cstdint>

namespace nexora::memory {

struct ManagedTabInfo {
    TabPriorityFactors factors;
    TabLifecycleState state = TabLifecycleState::Active;
    std::string favicon_url;
    int scroll_x = 0;
    int scroll_y = 0;
    std::vector<std::string> navigation_history;
    int current_history_index = -1;
    int64_t frozen_timestamp = 0;
    int64_t discarded_timestamp = 0;
};

class TabLifecycleManager {
public:
    using TabStateChangeCallback = std::function<void(int64_t tab_id, TabLifecycleState old_state, TabLifecycleState new_state)>;
    using TabDiscardCallback = std::function<void(int64_t tab_id, const DiscardedTabState& discarded_state)>;
    using TabRestoreCallback = std::function<void(int64_t tab_id, const DiscardedTabState& state_to_restore)>;

    static TabLifecycleManager& Instance() {
        static TabLifecycleManager instance;
        return instance;
    }

    void Init(int freeze_delay_seconds = 300, int discard_delay_seconds = 900);

    void RegisterTab(int64_t tab_id, int window_id, const std::string& url, const std::string& title);
    void UnregisterTab(int64_t tab_id);

    void SetTabActive(int64_t tab_id, bool active = true);
    void SetTabPinned(int64_t tab_id, bool pinned);
    void SetTabAudioPlaying(int64_t tab_id, bool playing);
    void SetTabVideoCall(int64_t tab_id, bool is_call);
    void SetTabProtected(int64_t tab_id, bool is_protected);
    void SetTabDownloadActive(int64_t tab_id, bool active);
    void SetTabFormInputActive(int64_t tab_id, bool has_input);

    void UpdateTabUrlAndTitle(int64_t tab_id, const std::string& url, const std::string& title, const std::string& favicon = "");
    void UpdateTabScroll(int64_t tab_id, int scroll_x, int scroll_y);
    void UpdateTabHistory(int64_t tab_id, const std::vector<std::string>& history, int current_idx);
    void UpdateTabMemory(int64_t tab_id, uint64_t memory_bytes);

    bool FreezeTab(int64_t tab_id);
    bool DiscardTab(int64_t tab_id);
    bool RestoreTab(int64_t tab_id);

    // Lifecycle evaluation loop (runs under timer or memory pressure)
    void PerformLifecycleCheck();

    TabLifecycleState GetTabState(int64_t tab_id) const;
    bool GetDiscardedState(int64_t tab_id, DiscardedTabState& out_state) const;
    ManagedTabInfo GetTabInfo(int64_t tab_id) const;
    std::vector<ManagedTabInfo> GetAllTabs() const;

    MemoryStats GetMemoryStats() const;

    void SetCallbacks(TabStateChangeCallback state_cb, TabDiscardCallback discard_cb, TabRestoreCallback restore_cb);

    void SetFreezeDelay(int seconds) { freeze_delay_seconds_ = seconds; }
    void SetDiscardDelay(int seconds) { discard_delay_seconds_ = seconds; }

private:
    TabLifecycleManager();
    ~TabLifecycleManager() = default;

    mutable std::mutex mutex_;
    std::map<int64_t, ManagedTabInfo> tabs_;
    std::map<int64_t, DiscardedTabState> discarded_states_;

    int freeze_delay_seconds_ = 300;
    int discard_delay_seconds_ = 900;
    uint64_t total_memory_saved_bytes_ = 0;

    TabStateChangeCallback state_change_cb_;
    TabDiscardCallback discard_cb_;
    TabRestoreCallback restore_cb_;
};

} // namespace nexora::memory
