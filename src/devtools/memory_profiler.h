#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace nexora::devtools {

struct MemorySnapshot {
    int64_t timestamp = 0;
    uint64_t total_working_set_bytes = 0;
    uint64_t total_private_bytes = 0;
    uint64_t v8_heap_used_bytes = 0;
    uint64_t v8_heap_total_bytes = 0;
    uint64_t blink_gc_bytes = 0;
    uint64_t dom_nodes_count = 0;
    uint64_t layout_cache_bytes = 0;
    uint64_t image_cache_bytes = 0;
    uint64_t code_cache_bytes = 0;
    uint64_t total_memory_saved_bytes = 0;
};

class MemoryProfiler {
public:
    static MemoryProfiler& Instance() {
        static MemoryProfiler instance;
        return instance;
    }

    MemorySnapshot TakeSnapshot();
    std::vector<MemorySnapshot> GetHistory() const;
    void ClearHistory();

    std::string ExportJson() const;

private:
    MemoryProfiler() = default;
    ~MemoryProfiler() = default;

    mutable std::mutex mutex_;
    std::vector<MemorySnapshot> history_;
};

} // namespace nexora::devtools
