#pragma once

#include "../util/win32_util.h"
#include <map>
#include <mutex>
#include <cstdint>

namespace nexora::devtools {

class CpuMonitor {
public:
    static CpuMonitor& Instance() {
        static CpuMonitor instance;
        return instance;
    }

    double SampleProcessCpuPercent(uint32_t pid);
    double SampleBrowserCpuPercent();

private:
    CpuMonitor() = default;
    ~CpuMonitor() = default;

    mutable std::mutex mutex_;
    std::map<uint32_t, util::CpuTimes> prev_times_;
};

} // namespace nexora::devtools
