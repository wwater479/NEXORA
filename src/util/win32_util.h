#pragma once

#include <string>
#include <cstdint>

#if defined(_WIN32)
#include <windows.h>
#else
// Type aliases for cross-platform portability
using HWND = void*;
using HINSTANCE = void*;
using HMENU = void*;
using HDC = void*;
using DWORD = uint32_t;
using BOOL = int;
#endif

namespace nexora::util {

struct MemoryInfo {
    uint64_t working_set_bytes = 0;
    uint64_t peak_working_set_bytes = 0;
    uint64_t private_bytes = 0;
    uint64_t commit_bytes = 0;
};

struct SystemMemoryInfo {
    uint64_t total_physical_bytes = 0;
    uint64_t avail_physical_bytes = 0;
    uint64_t total_pagefile_bytes = 0;
    uint64_t avail_pagefile_bytes = 0;
    uint32_t memory_load_percent = 0;
};

struct CpuTimes {
    uint64_t user_time_ns = 0;
    uint64_t kernel_time_ns = 0;
    uint64_t wall_time_ns = 0;
};

class Win32Util {
public:
    static void EnableDpiAwareness();
    static double GetDpiScaleForHwnd(HWND hwnd);

    static bool EnableDarkModeForHwnd(HWND hwnd, bool enable = true);

    // Process memory queries
    static bool GetProcessMemory(uint32_t process_id, MemoryInfo& out_info);
    static bool GetCurrentProcessMemory(MemoryInfo& out_info);

    // System memory queries
    static bool GetSystemMemory(SystemMemoryInfo& out_info);

    // CPU sampling
    static bool GetProcessCpuTimes(uint32_t process_id, CpuTimes& out_times);
    static bool GetCurrentProcessCpuTimes(CpuTimes& out_times);

    // Single-instance named mutex check
    static bool AcquireSingleInstanceLock(const std::string& mutex_name);
    static void ReleaseSingleInstanceLock();
};

} // namespace nexora::util
