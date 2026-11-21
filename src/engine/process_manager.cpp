#include "process_manager.h"
#include "../util/logger.h"
#include "../devtools/gpu_monitor.h"

namespace nexora::engine {

void ProcessManager::RegisterProcess(uint32_t pid, const std::string& process_type, int64_t tab_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        if (tab_id >= 0) {
            auto& t_vec = it->second.associated_tab_ids;
            if (std::find(t_vec.begin(), t_vec.end(), tab_id) == t_vec.end()) {
                t_vec.push_back(tab_id);
            }
        }
        LOG_INFO << "Process updated: PID=" << pid << " (" << process_type << ", total tabs=" << it->second.associated_tab_ids.size() << ")";
    } else {
        ProcessEntry entry;
        entry.pid = pid;
        entry.process_type = process_type;
        if (tab_id >= 0) {
            entry.associated_tab_ids.push_back(tab_id);
        }
        entry.is_crashed = false;
        processes_[pid] = entry;
        LOG_INFO << "Process registered: PID=" << pid << " (" << process_type << ", tab=" << tab_id << ")";
    }
}

bool ProcessManager::UnregisterTabFromProcess(uint32_t pid, int64_t tab_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) return false;

    auto& t_vec = it->second.associated_tab_ids;
    auto pos = std::find(t_vec.begin(), t_vec.end(), tab_id);
    if (pos != t_vec.end()) {
        t_vec.erase(pos);
    }

    LOG_INFO << "Unregistered tab " << tab_id << " from PID " << pid << " (Remaining tabs on PID: " << t_vec.size() << ")";

    // If no more tabs are associated with this renderer process, it can be unregistered
    if (t_vec.empty()) {
        processes_.erase(it);
        LOG_INFO << "Process unregistered (no remaining tabs): PID=" << pid;
        return true; // Eligible for full OS process termination
    }

    return false; // Still shared by other active tabs; DO NOT terminate process!
}

void ProcessManager::UnregisterProcess(uint32_t pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    processes_.erase(pid);
    LOG_INFO << "Process unregistered: PID=" << pid;
}

void ProcessManager::HandleRendererCrash(uint32_t pid, int64_t tab_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        it->second.is_crashed = true;
    }
    LOG_ERROR << "Renderer process crash detected for tab " << tab_id << " (PID=" << pid << ")";
}

void ProcessManager::HandleGpuCrash() {
    devtools::GpuMonitor::Instance().RecordGpuCrash();
}

std::vector<ProcessEntry> ProcessManager::GetAllProcesses() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProcessEntry> list;
    for (const auto& [pid, entry] : processes_) {
        list.push_back(entry);
    }
    return list;
}

uint32_t ProcessManager::GetPidForTab(int64_t tab_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [pid, entry] : processes_) {
        for (int64_t tid : entry.associated_tab_ids) {
            if (tid == tab_id) return pid;
        }
    }
    return 0;
}

std::vector<int64_t> ProcessManager::GetTabsForProcess(uint32_t pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        return it->second.associated_tab_ids;
    }
    return {};
}

bool ProcessManager::IsProcessShared(uint32_t pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        return it->second.associated_tab_ids.size() > 1;
    }
    return false;
}

bool ProcessManager::CanSafelyTerminateProcess(uint32_t pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it != processes_.end()) {
        return it->second.associated_tab_ids.empty();
    }
    return true;
}

size_t ProcessManager::GetActiveRendererCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t count = 0;
    for (const auto& [pid, entry] : processes_) {
        if (entry.process_type == "renderer" && !entry.is_crashed) count++;
    }
    return count;
}

} // namespace nexora::engine
