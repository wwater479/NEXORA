#pragma once

#include "memory_types.h"
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

namespace nexora::memory {

struct ProcessMemoryReport {
    uint32_t pid = 0;
    std::string process_type; // "browser", "gpu", "utility", "renderer"
    std::string label;        // e.g. "Main Browser Window", "Tab: GitHub"
    int64_t tab_id = -1;
    TabLifecycleState lifecycle_state = TabLifecycleState::Active;
    uint64_t working_set_bytes = 0;
    uint64_t private_bytes = 0;
    uint64_t commit_bytes = 0;
    uint64_t gpu_memory_bytes = 0;
};

struct BrowserWideMemoryReport {
    uint64_t total_working_set_bytes = 0;
    uint64_t total_private_bytes = 0;
    uint64_t total_commit_bytes = 0;
    uint64_t total_gpu_memory_bytes = 0;

    // Process counts
    size_t browser_process_count = 1;
    size_t gpu_process_count = 1;
    size_t utility_process_count = 1;
    size_t renderer_process_count = 0;

    // Tab state distribution
    size_t total_tabs = 0;
    size_t active_tabs = 0;
    size_t background_tabs = 0;
    size_t frozen_tabs = 0;
    size_t discarded_tabs = 0;

    // Per-process details
    std::vector<ProcessMemoryReport> processes;
};

class MemoryAccounting {
public:
    static MemoryAccounting& Instance() {
        static MemoryAccounting instance;
        return instance;
    }

    // Generate accurate, browser-wide memory snapshot across all processes
    BrowserWideMemoryReport GenerateReport();

    // Measures actual memory delta before and after tab discard
    bool MeasureDiscardDelta(int64_t tab_id, uint64_t& out_ws_freed_bytes, uint64_t& out_priv_freed_bytes);

    // Differentiate Working Set Trimming vs Actual Memory Reclamation
    bool TrimWorkingSet(uint32_t pid);
    bool ReclaimRendererMemory(uint32_t pid, int64_t tab_id);

private:
    MemoryAccounting() = default;
    ~MemoryAccounting() = default;

    mutable std::mutex mutex_;
};

} // namespace nexora::memory
