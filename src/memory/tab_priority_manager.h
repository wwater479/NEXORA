#pragma once

#include "memory_types.h"
#include <vector>
#include <mutex>

namespace nexora::memory {

class TabPriorityManager {
public:
    static TabPriorityManager& Instance() {
        static TabPriorityManager instance;
        return instance;
    }

    double CalculatePriorityScore(const TabPriorityFactors& factors, MemoryPressureLevel pressure_level = MemoryPressureLevel::Normal) const;

    bool IsTabImmuneFromDiscard(const TabPriorityFactors& factors) const;

    // Returns tabs sorted from lowest priority (best discard candidates) to highest priority
    std::vector<TabPriorityFactors> GetDiscardCandidates(
        const std::vector<TabPriorityFactors>& all_tabs,
        MemoryPressureLevel pressure_level = MemoryPressureLevel::Normal) const;

private:
    TabPriorityManager() = default;
    ~TabPriorityManager() = default;
};

} // namespace nexora::memory
