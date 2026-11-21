#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <algorithm>
#include <cstdint>

namespace nexora::engine {

struct ProcessEntry {
    uint32_t pid = 0;
    std::string process_type; // "browser", "renderer", "gpu", "utility"
    std::vector<int64_t> associated_tab_ids;
    bool is_crashed = false;
    uint64_t working_set_bytes = 0;
    uint64_t private_bytes = 0;
};

class ProcessManager {
public:
    static ProcessManager& Instance() {
        static ProcessManager instance;
        return instance;
    }

    void RegisterProcess(uint32_t pid, const std::string& process_type, int64_t tab_id = -1);
    bool UnregisterTabFromProcess(uint32_t pid, int64_t tab_id);
    void UnregisterProcess(uint32_t pid);

    void HandleRendererCrash(uint32_t pid, int64_t tab_id);
    void HandleGpuCrash();

    std::vector<ProcessEntry> GetAllProcesses() const;
    uint32_t GetPidForTab(int64_t tab_id) const;
    std::vector<int64_t> GetTabsForProcess(uint32_t pid) const;

    bool IsProcessShared(uint32_t pid) const;
    bool CanSafelyTerminateProcess(uint32_t pid) const;

    size_t GetActiveRendererCount() const;

private:
    ProcessManager() = default;
    ~ProcessManager() = default;

    mutable std::mutex mutex_;
    std::map<uint32_t, ProcessEntry> processes_;
};

} // namespace nexora::engine
