#pragma once

#include "memory_types.h"
#include <vector>
#include <functional>
#include <mutex>

namespace nexora::memory {

class MemoryPressureController {
public:
    using PressureChangeCallback = std::function<void(MemoryPressureLevel old_level, MemoryPressureLevel new_level)>;

    static MemoryPressureController& Instance() {
        static MemoryPressureController instance;
        return instance;
    }

    void Init(int moderate_threshold_percent = 80, int critical_threshold_percent = 90);

    MemoryPressureLevel EvaluatePressure();
    MemoryPressureLevel GetCurrentPressureLevel() const { return current_level_; }

    void RegisterPressureListener(PressureChangeCallback callback);

    void SetThresholds(int moderate_threshold_percent, int critical_threshold_percent);
    int GetModerateThreshold() const { return moderate_threshold_percent_; }
    int GetCriticalThreshold() const { return critical_threshold_percent_; }

private:
    MemoryPressureController();
    ~MemoryPressureController() = default;

    mutable std::mutex mutex_;
    MemoryPressureLevel current_level_ = MemoryPressureLevel::Normal;
    int moderate_threshold_percent_ = 80;
    int critical_threshold_percent_ = 90;
    std::vector<PressureChangeCallback> listeners_;
};

} // namespace nexora::memory
