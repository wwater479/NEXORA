#include "gpu_monitor.h"
#include "../util/logger.h"

namespace nexora::devtools {

GpuInfo GpuMonitor::GetGpuInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return info_;
}

void GpuMonitor::RecordGpuCrash() {
    std::lock_guard<std::mutex> lock(mutex_);
    info_.crash_restart_count++;
    LOG_WARN << "GPU process crash detected. Restarts: " << info_.crash_restart_count;
}

void GpuMonitor::SetGpuMemoryUsage(uint64_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    info_.current_gpu_memory_used_bytes = bytes;
}

} // namespace nexora::devtools
