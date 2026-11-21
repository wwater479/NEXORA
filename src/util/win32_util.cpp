#include "win32_util.h"
#include <chrono>

#if defined(_WIN32)
#include <windows.h>
#include <psapi.h>
#include <dwmapi.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static HANDLE g_single_instance_mutex = nullptr;

#else
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

namespace nexora::util {

void Win32Util::EnableDpiAwareness() {
#if defined(_WIN32)
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#endif
}

double Win32Util::GetDpiScaleForHwnd(HWND hwnd) {
#if defined(_WIN32)
    if (hwnd) {
        UINT dpi = GetDpiForWindow((HWND)hwnd);
        if (dpi > 0) return static_cast<double>(dpi) / 96.0;
    }
    HDC screen = GetDC(nullptr);
    int dpi_x = GetDeviceCaps(screen, LOGPIXELSX);
    ReleaseDC(nullptr, screen);
    return static_cast<double>(dpi_x) / 96.0;
#else
    (void)hwnd;
    return 1.0;
#endif
}

bool Win32Util::EnableDarkModeForHwnd(HWND hwnd, bool enable) {
#if defined(_WIN32)
    if (!hwnd) return false;
    BOOL value = enable ? TRUE : FALSE;
    HRESULT hr = DwmSetWindowAttribute((HWND)hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    return SUCCEEDED(hr);
#else
    (void)hwnd;
    (void)enable;
    return true;
#endif
}

bool Win32Util::GetProcessMemory(uint32_t process_id, MemoryInfo& out_info) {
#if defined(_WIN32)
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, process_id);
    if (!h_process) return false;
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(h_process, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        out_info.working_set_bytes = pmc.WorkingSetSize;
        out_info.peak_working_set_bytes = pmc.PeakWorkingSetSize;
        out_info.private_bytes = pmc.PrivateUsage;
        out_info.commit_bytes = pmc.PagefileUsage;
        CloseHandle(h_process);
        return true;
    }
    CloseHandle(h_process);
    return false;
#else
    std::string statm_path = "/proc/" + std::to_string(process_id) + "/statm";
    std::ifstream statm_file(statm_path);
    if (statm_file.is_open()) {
        unsigned long size_pages = 0, resident_pages = 0, shared_pages = 0;
        statm_file >> size_pages >> resident_pages >> shared_pages;
        long page_size = sysconf(_SC_PAGESIZE);
        out_info.working_set_bytes = resident_pages * page_size;
        out_info.peak_working_set_bytes = resident_pages * page_size;
        unsigned long private_pages = (resident_pages > shared_pages) ? (resident_pages - shared_pages) : resident_pages;
        out_info.private_bytes = private_pages * page_size;
        out_info.commit_bytes = size_pages * page_size;
        return true;
    }
    return false;
#endif
}

bool Win32Util::GetCurrentProcessMemory(MemoryInfo& out_info) {
#if defined(_WIN32)
    return GetProcessMemory(GetCurrentProcessId(), out_info);
#else
    return GetProcessMemory(static_cast<uint32_t>(getpid()), out_info);
#endif
}

bool Win32Util::GetSystemMemory(SystemMemoryInfo& out_info) {
#if defined(_WIN32)
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    if (GlobalMemoryStatusEx(&mem_status)) {
        out_info.total_physical_bytes = mem_status.ullTotalPhys;
        out_info.avail_physical_bytes = mem_status.ullAvailPhys;
        out_info.total_pagefile_bytes = mem_status.ullTotalPageFile;
        out_info.avail_pagefile_bytes = mem_status.ullAvailPageFile;
        out_info.memory_load_percent = mem_status.dwMemoryLoad;
        return true;
    }
    return false;
#else
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        uint64_t mem_total_kb = 0, mem_avail_kb = 0, mem_free_kb = 0;
        while (std::getline(meminfo, line)) {
            if (line.rfind("MemTotal:", 0) == 0) {
                std::istringstream ss(line.substr(9));
                ss >> mem_total_kb;
            } else if (line.rfind("MemAvailable:", 0) == 0) {
                std::istringstream ss(line.substr(13));
                ss >> mem_avail_kb;
            } else if (line.rfind("MemFree:", 0) == 0) {
                std::istringstream ss(line.substr(8));
                ss >> mem_free_kb;
            }
        }
        if (mem_avail_kb == 0) mem_avail_kb = mem_free_kb;
        out_info.total_physical_bytes = mem_total_kb * 1024;
        out_info.avail_physical_bytes = mem_avail_kb * 1024;
        out_info.total_pagefile_bytes = out_info.total_physical_bytes * 2;
        out_info.avail_pagefile_bytes = out_info.avail_physical_bytes * 2;
        if (out_info.total_physical_bytes > 0) {
            uint64_t used = out_info.total_physical_bytes - out_info.avail_physical_bytes;
            out_info.memory_load_percent = static_cast<uint32_t>((used * 100) / out_info.total_physical_bytes);
        }
        return true;
    }
    return false;
#endif
}

bool Win32Util::GetProcessCpuTimes(uint32_t process_id, CpuTimes& out_times) {
    auto now = std::chrono::steady_clock::now();
    out_times.wall_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

#if defined(_WIN32)
    HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, process_id);
    if (!h_process) return false;
    FILETIME creation_time, exit_time, kernel_time, user_time;
    if (GetProcessTimes(h_process, &creation_time, &exit_time, &kernel_time, &user_time)) {
        ULARGE_INTEGER kt, ut;
        kt.LowPart = kernel_time.dwLowDateTime;
        kt.HighPart = kernel_time.dwHighDateTime;
        ut.LowPart = user_time.dwLowDateTime;
        ut.HighPart = user_time.dwHighDateTime;
        // Convert 100-ns intervals to nanoseconds
        out_times.kernel_time_ns = kt.QuadPart * 100;
        out_times.user_time_ns = ut.QuadPart * 100;
        CloseHandle(h_process);
        return true;
    }
    CloseHandle(h_process);
    return false;
#else
    std::string stat_path = "/proc/" + std::to_string(process_id) + "/stat";
    std::ifstream stat_file(stat_path);
    if (stat_file.is_open()) {
        std::string comm;
        int pid;
        char state;
        int ppid, pgrp, session, tty_nr, tpgid;
        unsigned int flags;
        unsigned long minflt, cminflt, majflt, cmajflt, utime, stime;
        stat_file >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr >> tpgid
                  >> flags >> minflt >> cminflt >> majflt >> cmajflt >> utime >> stime;
        long ticks_per_sec = sysconf(_SC_CLK_TCK);
        if (ticks_per_sec > 0) {
            out_times.user_time_ns = (utime * 1000000000ULL) / ticks_per_sec;
            out_times.kernel_time_ns = (stime * 1000000000ULL) / ticks_per_sec;
            return true;
        }
    }
    return false;
#endif
}

bool Win32Util::GetCurrentProcessCpuTimes(CpuTimes& out_times) {
#if defined(_WIN32)
    return GetProcessCpuTimes(GetCurrentProcessId(), out_times);
#else
    return GetProcessCpuTimes(static_cast<uint32_t>(getpid()), out_times);
#endif
}

bool Win32Util::AcquireSingleInstanceLock(const std::string& mutex_name) {
#if defined(_WIN32)
    std::wstring wname(mutex_name.begin(), mutex_name.end());
    g_single_instance_mutex = CreateMutexW(nullptr, TRUE, wname.c_str());
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (g_single_instance_mutex) {
            CloseHandle(g_single_instance_mutex);
            g_single_instance_mutex = nullptr;
        }
        return false;
    }
    return true;
#else
    (void)mutex_name;
    return true;
#endif
}

void Win32Util::ReleaseSingleInstanceLock() {
#if defined(_WIN32)
    if (g_single_instance_mutex) {
        ReleaseMutex(g_single_instance_mutex);
        CloseHandle(g_single_instance_mutex);
        g_single_instance_mutex = nullptr;
    }
#endif
}

} // namespace nexora::util
