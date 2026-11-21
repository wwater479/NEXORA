#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace nexora::memory {

enum class TabLifecycleState {
    Active,         // Currently selected tab, full performance, highest priority
    Background,     // Unselected tab, normal functionality, throttled timers
    Frozen,         // Idle background tab, suspended execution, state cached
    Discarded       // Renderer freed, URL/history/scroll state preserved, restored on click
};

inline const char* TabLifecycleStateToString(TabLifecycleState state) {
    switch (state) {
        case TabLifecycleState::Active:     return "ACTIVE";
        case TabLifecycleState::Background: return "BACKGROUND";
        case TabLifecycleState::Frozen:     return "FROZEN";
        case TabLifecycleState::Discarded:  return "DISCARDED";
    }
    return "ACTIVE";
}

enum class MemoryPressureLevel {
    Normal,         // Available RAM > 25%, normal operation
    Moderate,       // Available RAM < 20%, accelerate freezing, discard idle tabs
    Critical        // Available RAM < 10%, immediate freeze, aggressive discarding & GC
};

inline const char* MemoryPressureLevelToString(MemoryPressureLevel level) {
    switch (level) {
        case MemoryPressureLevel::Normal:   return "NORMAL";
        case MemoryPressureLevel::Moderate: return "MODERATE";
        case MemoryPressureLevel::Critical: return "CRITICAL";
    }
    return "NORMAL";
}

struct TabPriorityFactors {
    int64_t tab_id = 0;
    int window_id = 0;
    std::string url;
    std::string title;
    bool is_active = false;
    bool is_pinned = false;
    bool is_audio_playing = false;
    bool is_video_call = false;
    bool has_active_download = false;
    bool is_user_protected = false;
    bool is_form_input_active = false;
    int64_t last_active_timestamp = 0; // seconds since epoch
    uint64_t estimated_memory_bytes = 0;
    double priority_score = 0.0;
};

struct DiscardedTabState {
    int64_t tab_id = 0;
    int window_id = 0;
    std::string url;
    std::string title;
    std::string favicon_url;
    int scroll_x = 0;
    int scroll_y = 0;
    std::vector<std::string> navigation_history;
    int current_history_index = -1;
    int64_t discarded_timestamp = 0;
    uint64_t memory_freed_bytes = 0;
};

struct MemoryStats {
    uint64_t total_browser_working_set_bytes = 0;
    uint64_t total_browser_private_bytes = 0;
    uint64_t total_memory_saved_bytes = 0;
    int active_tabs_count = 0;
    int background_tabs_count = 0;
    int frozen_tabs_count = 0;
    int discarded_tabs_count = 0;
    int system_memory_load_percent = 0;
    uint64_t system_total_phys_bytes = 0;
    uint64_t system_avail_phys_bytes = 0;
    MemoryPressureLevel current_pressure_level = MemoryPressureLevel::Normal;
};

} // namespace nexora::memory
