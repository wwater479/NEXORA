#include "tab_lifecycle_manager.h"
#include "tab_priority_manager.h"
#include "memory_pressure_controller.h"
#include "system_memory_monitor.h"
#include "../util/logger.h"
#include <chrono>

namespace nexora::memory {

TabLifecycleManager::TabLifecycleManager() {}

void TabLifecycleManager::Init(int freeze_delay_seconds, int discard_delay_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    freeze_delay_seconds_ = freeze_delay_seconds;
    discard_delay_seconds_ = discard_delay_seconds;

    MemoryPressureController::Instance().RegisterPressureListener(
        [this](MemoryPressureLevel old_level, MemoryPressureLevel new_level) {
            (void)old_level;
            if (new_level == MemoryPressureLevel::Moderate || new_level == MemoryPressureLevel::Critical) {
                PerformLifecycleCheck();
            }
        });
}

void TabLifecycleManager::SetCallbacks(TabStateChangeCallback state_cb, TabDiscardCallback discard_cb, TabRestoreCallback restore_cb) {
    std::lock_guard<std::mutex> lock(mutex_);
    state_change_cb_ = state_cb;
    discard_cb_ = discard_cb;
    restore_cb_ = restore_cb;
}

void TabLifecycleManager::RegisterTab(int64_t tab_id, int window_id, const std::string& url, const std::string& title) {
    std::lock_guard<std::mutex> lock(mutex_);
    ManagedTabInfo info;
    info.factors.tab_id = tab_id;
    info.factors.window_id = window_id;
    info.factors.url = url;
    info.factors.title = title;
    info.factors.is_active = true;
    info.factors.last_active_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    info.factors.estimated_memory_bytes = 45 * 1024 * 1024; // Baseline 45MB per active tab
    info.state = TabLifecycleState::Active;
    if (!url.empty()) {
        info.navigation_history.push_back(url);
        info.current_history_index = 0;
    }

    tabs_[tab_id] = info;
    LOG_INFO << "Registered tab " << tab_id << " (" << title << ")";
}

void TabLifecycleManager::UnregisterTab(int64_t tab_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    tabs_.erase(tab_id);
    discarded_states_.erase(tab_id);
    LOG_INFO << "Unregistered tab " << tab_id;
}

void TabLifecycleManager::SetTabActive(int64_t tab_id, bool active) {
    TabStateChangeCallback cb;
    TabRestoreCallback r_cb;
    DiscardedTabState restore_data;
    TabLifecycleState old_state = TabLifecycleState::Active;
    TabLifecycleState new_state = TabLifecycleState::Active;
    bool needs_restore = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tabs_.find(tab_id);
        if (it == tabs_.end()) return;

        old_state = it->second.state;
        it->second.factors.is_active = active;

        if (active) {
            it->second.factors.last_active_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            if (old_state == TabLifecycleState::Discarded) {
                needs_restore = true;
                auto d_it = discarded_states_.find(tab_id);
                if (d_it != discarded_states_.end()) {
                    restore_data = d_it->second;
                    discarded_states_.erase(d_it);
                }
            }

            it->second.state = TabLifecycleState::Active;
            new_state = TabLifecycleState::Active;
        } else {
            if (old_state == TabLifecycleState::Active) {
                it->second.state = TabLifecycleState::Background;
                new_state = TabLifecycleState::Background;
            } else {
                new_state = old_state;
            }
        }

        cb = state_change_cb_;
        r_cb = restore_cb_;
    }

    if (needs_restore && r_cb) {
        LOG_INFO << "Restoring discarded tab " << tab_id << " -> URL: " << restore_data.url;
        r_cb(tab_id, restore_data);
    }

    if (old_state != new_state && cb) {
        cb(tab_id, old_state, new_state);
    }
}

void TabLifecycleManager::SetTabPinned(int64_t tab_id, bool pinned) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) it->second.factors.is_pinned = pinned;
}

void TabLifecycleManager::SetTabAudioPlaying(int64_t tab_id, bool playing) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) it->second.factors.is_audio_playing = playing;
}

void TabLifecycleManager::SetTabVideoCall(int64_t tab_id, bool is_call) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) it->second.factors.is_video_call = is_call;
}

void TabLifecycleManager::SetTabProtected(int64_t tab_id, bool is_protected) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) it->second.factors.is_user_protected = is_protected;
}

void TabLifecycleManager::SetTabDownloadActive(int64_t tab_id, bool active) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) it->second.factors.has_active_download = active;
}

void TabLifecycleManager::SetTabFormInputActive(int64_t tab_id, bool has_input) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) it->second.factors.is_form_input_active = has_input;
}

void TabLifecycleManager::UpdateTabUrlAndTitle(int64_t tab_id, const std::string& url, const std::string& title, const std::string& favicon) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) {
        it->second.factors.url = url;
        it->second.factors.title = title;
        if (!favicon.empty()) it->second.favicon_url = favicon;

        if (!url.empty()) {
            if (it->second.navigation_history.empty() || it->second.navigation_history.back() != url) {
                it->second.navigation_history.push_back(url);
                it->second.current_history_index = static_cast<int>(it->second.navigation_history.size()) - 1;
            }
        }
    }
}

void TabLifecycleManager::UpdateTabScroll(int64_t tab_id, int scroll_x, int scroll_y) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) {
        it->second.scroll_x = scroll_x;
        it->second.scroll_y = scroll_y;
    }
}

void TabLifecycleManager::UpdateTabHistory(int64_t tab_id, const std::vector<std::string>& history, int current_idx) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) {
        it->second.navigation_history = history;
        it->second.current_history_index = current_idx;
    }
}

void TabLifecycleManager::UpdateTabMemory(int64_t tab_id, uint64_t memory_bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) {
        it->second.factors.estimated_memory_bytes = memory_bytes;
    }
}

bool TabLifecycleManager::FreezeTab(int64_t tab_id) {
    TabStateChangeCallback cb;
    TabLifecycleState old_state = TabLifecycleState::Background;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tabs_.find(tab_id);
        if (it == tabs_.end() || it->second.factors.is_active || it->second.state == TabLifecycleState::Discarded || it->second.state == TabLifecycleState::Frozen) {
            return false;
        }

        old_state = it->second.state;
        it->second.state = TabLifecycleState::Frozen;
        it->second.frozen_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        cb = state_change_cb_;
    }

    LOG_INFO << "Tab " << tab_id << " transitioned to FROZEN";
    if (cb) cb(tab_id, old_state, TabLifecycleState::Frozen);
    return true;
}

bool TabLifecycleManager::DiscardTab(int64_t tab_id) {
    TabStateChangeCallback cb;
    TabDiscardCallback d_cb;
    DiscardedTabState d_state;
    TabLifecycleState old_state = TabLifecycleState::Background;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tabs_.find(tab_id);
        if (it == tabs_.end() || it->second.factors.is_active || it->second.state == TabLifecycleState::Discarded) {
            return false;
        }

        if (TabPriorityManager::Instance().IsTabImmuneFromDiscard(it->second.factors)) {
            LOG_DEBUG << "Cannot discard immune tab " << tab_id;
            return false;
        }

        old_state = it->second.state;
        it->second.state = TabLifecycleState::Discarded;
        it->second.discarded_timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        d_state.tab_id = tab_id;
        d_state.window_id = it->second.factors.window_id;
        d_state.url = it->second.factors.url;
        d_state.title = it->second.factors.title;
        d_state.favicon_url = it->second.favicon_url;
        d_state.scroll_x = it->second.scroll_x;
        d_state.scroll_y = it->second.scroll_y;
        d_state.navigation_history = it->second.navigation_history;
        d_state.current_history_index = it->second.current_history_index;
        d_state.discarded_timestamp = it->second.discarded_timestamp;
        d_state.memory_freed_bytes = it->second.factors.estimated_memory_bytes;

        total_memory_saved_bytes_ += d_state.memory_freed_bytes;
        discarded_states_[tab_id] = d_state;
        cb = state_change_cb_;
        d_cb = discard_cb_;
    }

    LOG_INFO << "Tab " << tab_id << " DISCARDED -> freed ~" << (d_state.memory_freed_bytes / (1024 * 1024)) << " MB RAM";
    if (d_cb) d_cb(tab_id, d_state);
    if (cb) cb(tab_id, old_state, TabLifecycleState::Discarded);
    return true;
}

bool TabLifecycleManager::RestoreTab(int64_t tab_id) {
    SetTabActive(tab_id, true);
    return true;
}

void TabLifecycleManager::PerformLifecycleCheck() {
    MemoryPressureLevel pressure = MemoryPressureController::Instance().EvaluatePressure();
    int64_t now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    std::vector<TabPriorityFactors> all_factors;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [id, tab] : tabs_) {
            all_factors.push_back(tab.factors);
        }
    }

    auto candidates = TabPriorityManager::Instance().GetDiscardCandidates(all_factors, pressure);

    for (const auto& candidate : candidates) {
        int64_t idle_time = now_ts - candidate.last_active_timestamp;
        int freeze_threshold = freeze_delay_seconds_;
        int discard_threshold = discard_delay_seconds_;

        if (pressure == MemoryPressureLevel::Moderate) {
            freeze_threshold = freeze_threshold / 2;
            discard_threshold = discard_threshold / 2;
        } else if (pressure == MemoryPressureLevel::Critical) {
            freeze_threshold = 0; // Immediate freeze
            discard_threshold = 60; // Discard idle tabs over 1 min
        }

        TabLifecycleState cur_state = GetTabState(candidate.tab_id);

        if (pressure == MemoryPressureLevel::Critical) {
            if (cur_state != TabLifecycleState::Discarded) {
                DiscardTab(candidate.tab_id);
            }
        } else if (idle_time >= discard_threshold) {
            if (cur_state != TabLifecycleState::Discarded) {
                DiscardTab(candidate.tab_id);
            }
        } else if (idle_time >= freeze_threshold) {
            if (cur_state == TabLifecycleState::Background) {
                FreezeTab(candidate.tab_id);
            }
        }
    }
}

TabLifecycleState TabLifecycleManager::GetTabState(int64_t tab_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) return it->second.state;
    return TabLifecycleState::Active;
}

bool TabLifecycleManager::GetDiscardedState(int64_t tab_id, DiscardedTabState& out_state) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = discarded_states_.find(tab_id);
    if (it != discarded_states_.end()) {
        out_state = it->second;
        return true;
    }
    return false;
}

ManagedTabInfo TabLifecycleManager::GetTabInfo(int64_t tab_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = tabs_.find(tab_id);
    if (it != tabs_.end()) return it->second;
    return ManagedTabInfo{};
}

std::vector<ManagedTabInfo> TabLifecycleManager::GetAllTabs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ManagedTabInfo> list;
    for (const auto& [id, tab] : tabs_) {
        list.push_back(tab);
    }
    return list;
}

MemoryStats TabLifecycleManager::GetMemoryStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    MemoryStats stats;
    stats.total_memory_saved_bytes = total_memory_saved_bytes_;

    for (const auto& [id, tab] : tabs_) {
        switch (tab.state) {
            case TabLifecycleState::Active:     stats.active_tabs_count++; break;
            case TabLifecycleState::Background: stats.background_tabs_count++; break;
            case TabLifecycleState::Frozen:     stats.frozen_tabs_count++; break;
            case TabLifecycleState::Discarded:  stats.discarded_tabs_count++; break;
        }
    }

    auto proc_mem = SystemMemoryMonitor::Instance().GetBrowserProcessMemory();
    stats.total_browser_working_set_bytes = proc_mem.working_set_bytes;
    stats.total_browser_private_bytes = proc_mem.private_bytes;

    auto sys_mem = SystemMemoryMonitor::Instance().GetSystemMemoryInfo();
    stats.system_total_phys_bytes = sys_mem.total_physical_bytes;
    stats.system_avail_phys_bytes = sys_mem.avail_physical_bytes;
    stats.system_memory_load_percent = sys_mem.memory_load_percent;
    stats.current_pressure_level = MemoryPressureController::Instance().GetCurrentPressureLevel();

    return stats;
}

} // namespace nexora::memory
