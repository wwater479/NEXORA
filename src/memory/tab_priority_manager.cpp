#include "tab_priority_manager.h"
#include <chrono>
#include <cmath>
#include <algorithm>

namespace nexora::memory {

bool TabPriorityManager::IsTabImmuneFromDiscard(const TabPriorityFactors& factors) const {
    if (factors.is_active) return true;
    if (factors.is_user_protected) return true;
    if (factors.is_audio_playing) return true;
    if (factors.is_video_call) return true;
    if (factors.has_active_download) return true;
    if (factors.is_form_input_active) return true;
    return false;
}

double TabPriorityManager::CalculatePriorityScore(const TabPriorityFactors& factors, MemoryPressureLevel pressure_level) const {
    if (factors.is_active) {
        return 1000.0;
    }

    double score = 0.0;

    if (factors.is_user_protected) {
        score += 500.0;
    }
    if (factors.is_audio_playing || factors.is_video_call) {
        score += 400.0;
    }
    if (factors.has_active_download) {
        score += 400.0;
    }
    if (factors.is_pinned) {
        score += 200.0;
    }
    if (factors.is_form_input_active) {
        score += 150.0;
    }

    // Recency score (up to 200 points based on idle time decay)
    int64_t now_ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t idle_seconds = now_ts - factors.last_active_timestamp;
    if (idle_seconds < 0) idle_seconds = 0;

    double decay = std::exp(-static_cast<double>(idle_seconds) / 300.0); // 5 min half-life
    score += decay * 200.0;

    // Memory penalty under critical pressure for bloated tabs (e.g. > 200MB)
    if (pressure_level == MemoryPressureLevel::Critical && factors.estimated_memory_bytes > 200 * 1024 * 1024) {
        double excess_mb = static_cast<double>(factors.estimated_memory_bytes - 200 * 1024 * 1024) / (1024.0 * 1024.0);
        score -= std::min(100.0, excess_mb * 0.5);
    }

    return std::max(0.0, score);
}

std::vector<TabPriorityFactors> TabPriorityManager::GetDiscardCandidates(
    const std::vector<TabPriorityFactors>& all_tabs,
    MemoryPressureLevel pressure_level) const {

    std::vector<TabPriorityFactors> candidates;
    for (auto tab : all_tabs) {
        if (!IsTabImmuneFromDiscard(tab)) {
            tab.priority_score = CalculatePriorityScore(tab, pressure_level);
            candidates.push_back(tab);
        }
    }

    // Sort ascending by score: lowest score = first to discard
    std::sort(candidates.begin(), candidates.end(), [](const TabPriorityFactors& a, const TabPriorityFactors& b) {
        return a.priority_score < b.priority_score;
    });

    return candidates;
}

} // namespace nexora::memory
