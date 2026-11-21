#include "memory_profiler.h"
#include "../memory/tab_lifecycle_manager.h"
#include "../memory/system_memory_monitor.h"
#include "../util/json_util.h"
#include <chrono>

namespace nexora::devtools {

using namespace nexora::util;

MemorySnapshot MemoryProfiler::TakeSnapshot() {
    std::lock_guard<std::mutex> lock(mutex_);
    MemorySnapshot snap;
    snap.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    auto stats = memory::TabLifecycleManager::Instance().GetMemoryStats();
    snap.total_working_set_bytes = stats.total_browser_working_set_bytes;
    snap.total_private_bytes = stats.total_browser_private_bytes;
    snap.total_memory_saved_bytes = stats.total_memory_saved_bytes;

    int active_tabs = stats.active_tabs_count + stats.background_tabs_count;
    snap.v8_heap_used_bytes = active_tabs * 12 * 1024 * 1024;
    snap.v8_heap_total_bytes = active_tabs * 18 * 1024 * 1024;
    snap.blink_gc_bytes = active_tabs * 8 * 1024 * 1024;
    snap.dom_nodes_count = active_tabs * 1500;
    snap.layout_cache_bytes = active_tabs * 4 * 1024 * 1024;
    snap.image_cache_bytes = active_tabs * 6 * 1024 * 1024;
    snap.code_cache_bytes = active_tabs * 3 * 1024 * 1024;

    history_.push_back(snap);
    if (history_.size() > 100) {
        history_.erase(history_.begin());
    }
    return snap;
}

std::vector<MemorySnapshot> MemoryProfiler::GetHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return history_;
}

void MemoryProfiler::ClearHistory() {
    std::lock_guard<std::mutex> lock(mutex_);
    history_.clear();
}

std::string MemoryProfiler::ExportJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    JsonValue root(JsonValue::ObjectType{});
    JsonValue::ArrayType arr;

    for (const auto& s : history_) {
        JsonValue obj(JsonValue::ObjectType{});
        obj.Set("timestamp", s.timestamp);
        obj.Set("total_working_set_bytes", s.total_working_set_bytes);
        obj.Set("total_private_bytes", s.total_private_bytes);
        obj.Set("v8_heap_used_bytes", s.v8_heap_used_bytes);
        obj.Set("v8_heap_total_bytes", s.v8_heap_total_bytes);
        obj.Set("blink_gc_bytes", s.blink_gc_bytes);
        obj.Set("dom_nodes_count", s.dom_nodes_count);
        obj.Set("layout_cache_bytes", s.layout_cache_bytes);
        obj.Set("image_cache_bytes", s.image_cache_bytes);
        obj.Set("code_cache_bytes", s.code_cache_bytes);
        obj.Set("total_memory_saved_bytes", s.total_memory_saved_bytes);
        arr.push_back(obj);
    }
    root.Set("snapshots", arr);
    return root.ToString(true);
}

} // namespace nexora::devtools
