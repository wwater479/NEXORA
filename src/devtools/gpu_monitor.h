#pragma once

#include <string>
#include <mutex>
#include <cstdint>

namespace nexora::devtools {

struct GpuInfo {
    std::string vendor = "NVIDIA / Intel / AMD";
    std::string renderer = "Direct3D 11 / D3D12";
    std::string driver_version = "31.0.15.2849";
    uint64_t dedicated_video_memory_bytes = 4ULL * 1024 * 1024 * 1024;
    uint64_t shared_system_memory_bytes = 8ULL * 1024 * 1024 * 1024;
    uint64_t current_gpu_memory_used_bytes = 120ULL * 1024 * 1024;
    bool is_hardware_accelerated = true;
    int crash_restart_count = 0;
};

class GpuMonitor {
public:
    static GpuMonitor& Instance() {
        static GpuMonitor instance;
        return instance;
    }

    GpuInfo GetGpuInfo() const;
    void RecordGpuCrash();
    void SetGpuMemoryUsage(uint64_t bytes);

private:
    GpuMonitor() = default;
    ~GpuMonitor() = default;

    mutable std::mutex mutex_;
    GpuInfo info_;
};

} // namespace nexora::devtools
