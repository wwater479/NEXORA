#pragma once

#include "memory_types.h"
#include "../util/win32_util.h"
#include <mutex>
#include <cstdint>

namespace nexora::memory {

class SystemMemoryMonitor {
public:
    static SystemMemoryMonitor& Instance() {
        static SystemMemoryMonitor instance;
        return instance;
    }

    void Sample();

    util::SystemMemoryInfo GetSystemMemoryInfo() const;
    util::MemoryInfo GetBrowserProcessMemory() const;

    uint32_t GetMemoryLoadPercent() const;
    uint64_t GetAvailablePhysicalBytes() const;
    uint64_t GetTotalPhysicalBytes() const;

    // Simulated memory pressure injection for benchmark and test suites
    void SetSimulatedSystemMemory(uint64_t total_bytes, uint64_t avail_bytes, uint32_t load_percent);
    void ClearSimulatedSystemMemory();

private:
    SystemMemoryMonitor();
    ~SystemMemoryMonitor() = default;

    mutable std::mutex mutex_;
    util::SystemMemoryInfo system_info_;
    util::MemoryInfo process_info_;
    bool has_simulated_memory_ = false;
};

} // namespace nexora::memory
