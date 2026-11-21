#include "memory_pressure_controller.h"
#include "system_memory_monitor.h"
#include "../util/logger.h"

namespace nexora::memory {

MemoryPressureController::MemoryPressureController() {}

void MemoryPressureController::Init(int moderate_threshold_percent, int critical_threshold_percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    moderate_threshold_percent_ = moderate_threshold_percent;
    critical_threshold_percent_ = critical_threshold_percent;
    current_level_ = MemoryPressureLevel::Normal;
}

void MemoryPressureController::SetThresholds(int moderate_threshold_percent, int critical_threshold_percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    moderate_threshold_percent_ = moderate_threshold_percent;
    critical_threshold_percent_ = critical_threshold_percent;
}

MemoryPressureLevel MemoryPressureController::EvaluatePressure() {
    SystemMemoryMonitor::Instance().Sample();
    uint32_t load = SystemMemoryMonitor::Instance().GetMemoryLoadPercent();

    MemoryPressureLevel new_level = MemoryPressureLevel::Normal;
    if (load >= static_cast<uint32_t>(critical_threshold_percent_)) {
        new_level = MemoryPressureLevel::Critical;
    } else if (load >= static_cast<uint32_t>(moderate_threshold_percent_)) {
        new_level = MemoryPressureLevel::Moderate;
    } else {
        new_level = MemoryPressureLevel::Normal;
    }

    std::vector<PressureChangeCallback> listeners_copy;
    MemoryPressureLevel old_level = current_level_;
    bool changed = false;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (new_level != current_level_) {
            changed = true;
            old_level = current_level_;
            current_level_ = new_level;
            listeners_copy = listeners_;
        }
    }

    if (changed) {
        LOG_INFO << "Memory pressure shifted: " << MemoryPressureLevelToString(old_level)
                 << " -> " << MemoryPressureLevelToString(new_level)
                 << " (System RAM load: " << load << "%)";
        for (const auto& cb : listeners_copy) {
            if (cb) cb(old_level, new_level);
        }
    }

    return new_level;
}

void MemoryPressureController::RegisterPressureListener(PressureChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    listeners_.push_back(callback);
}

} // namespace nexora::memory
