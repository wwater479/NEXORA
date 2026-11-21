#include "cpu_monitor.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace nexora::devtools {

double CpuMonitor::SampleProcessCpuPercent(uint32_t pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    util::CpuTimes cur_times;
    if (!util::Win32Util::GetProcessCpuTimes(pid, cur_times)) {
        return 0.0;
    }

    auto it = prev_times_.find(pid);
    if (it == prev_times_.end()) {
        prev_times_[pid] = cur_times;
        return 0.0;
    }

    uint64_t delta_user = cur_times.user_time_ns - it->second.user_time_ns;
    uint64_t delta_kernel = cur_times.kernel_time_ns - it->second.kernel_time_ns;
    uint64_t delta_wall = cur_times.wall_time_ns - it->second.wall_time_ns;
    it->second = cur_times;

    if (delta_wall == 0) return 0.0;

    double cpu_percent = static_cast<double>(delta_user + delta_kernel) * 100.0 / static_cast<double>(delta_wall);
    return std::max(0.0, std::min(100.0, cpu_percent));
}

double CpuMonitor::SampleBrowserCpuPercent() {
#if defined(_WIN32)
    return SampleProcessCpuPercent(GetCurrentProcessId());
#else
    return SampleProcessCpuPercent(static_cast<uint32_t>(getpid()));
#endif
}

} // namespace nexora::devtools
