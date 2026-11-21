#pragma once

#include "../memory/memory_types.h"
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>

namespace nexora::devtools {

enum class ProcessType {
    Browser,
    GPU,
    NetworkService,
    RendererTab,
    Utility
};

inline const char* ProcessTypeToString(ProcessType type) {
    switch (type) {
        case ProcessType::Browser:        return "Browser";
        case ProcessType::GPU:            return "GPU Process";
        case ProcessType::NetworkService: return "Network Service";
        case ProcessType::RendererTab:    return "Tab";
        case ProcessType::Utility:        return "Utility";
    }
    return "Unknown";
}

struct TaskProcessInfo {
    std::string task_name;
    ProcessType type = ProcessType::RendererTab;
    uint32_t pid = 0;
    int64_t tab_id = -1;
    memory::TabLifecycleState lifecycle_state = memory::TabLifecycleState::Active;
    uint64_t private_bytes = 0;
    uint64_t working_set_bytes = 0;
    double cpu_percent = 0.0;
    uint64_t gpu_memory_bytes = 0;
    uint64_t network_rx_bytes_sec = 0;
    bool is_protected = false;
};

enum class TaskSortColumn {
    Name,
    State,
    PID,
    PrivateBytes,
    WorkingSet,
    CPU,
    Network
};

class TaskManager {
public:
    static TaskManager& Instance() {
        static TaskManager instance;
        return instance;
    }

    void StartMonitoring(int interval_ms = 1000);
    void StopMonitoring();
    bool IsMonitoring() const { return is_monitoring_; }

    void RefreshTasks();
    std::vector<TaskProcessInfo> GetTasks(TaskSortColumn sort_col = TaskSortColumn::PrivateBytes, bool ascending = false) const;

    bool EndProcess(uint32_t pid);
    bool ForceDiscardTab(int64_t tab_id);
    bool ForceFreezeTab(int64_t tab_id);
    bool ToggleTabProtection(int64_t tab_id);
    bool RunGarbageCollection(int64_t tab_id = -1);

    void RegisterTabProcess(int64_t tab_id, uint32_t pid, const std::string& title);
    void UnregisterTabProcess(int64_t tab_id);

private:
    TaskManager();
    ~TaskManager();

    mutable std::mutex mutex_;
    std::vector<TaskProcessInfo> tasks_;
    std::map<int64_t, uint32_t> tab_pids_;
    bool is_monitoring_ = false;
    int interval_ms_ = 1000;
};

} // namespace nexora::devtools
