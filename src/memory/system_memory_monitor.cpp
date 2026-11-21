#include "system_memory_monitor.h"

namespace nexora::memory {

SystemMemoryMonitor::SystemMemoryMonitor() {
    Sample();
}

void SystemMemoryMonitor::Sample() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!has_simulated_memory_) {
        util::Win32Util::GetSystemMemory(system_info_);
    }
    util::Win32Util::GetCurrentProcessMemory(process_info_);
}

util::SystemMemoryInfo SystemMemoryMonitor::GetSystemMemoryInfo() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return system_info_;
}

util::MemoryInfo SystemMemoryMonitor::GetBrowserProcessMemory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return process_info_;
}

uint32_t SystemMemoryMonitor::GetMemoryLoadPercent() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return system_info_.memory_load_percent;
}

uint64_t SystemMemoryMonitor::GetAvailablePhysicalBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return system_info_.avail_physical_bytes;
}

uint64_t SystemMemoryMonitor::GetTotalPhysicalBytes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return system_info_.total_physical_bytes;
}

void SystemMemoryMonitor::SetSimulatedSystemMemory(uint64_t total_bytes, uint64_t avail_bytes, uint32_t load_percent) {
    std::lock_guard<std::mutex> lock(mutex_);
    has_simulated_memory_ = true;
    system_info_.total_physical_bytes = total_bytes;
    system_info_.avail_physical_bytes = avail_bytes;
    system_info_.memory_load_percent = load_percent;
}

void SystemMemoryMonitor::ClearSimulatedSystemMemory() {
    std::lock_guard<std::mutex> lock(mutex_);
    has_simulated_memory_ = false;
    util::Win32Util::GetSystemMemory(system_info_);
}

} // namespace nexora::memory
