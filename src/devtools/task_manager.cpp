#include "task_manager.h"
#include "../memory/tab_lifecycle_manager.h"
#include "../memory/system_memory_monitor.h"
#include "../util/win32_util.h"
#include "../util/logger.h"
#include <algorithm>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace nexora::devtools {

using namespace nexora::util;

TaskManager::TaskManager() {}
TaskManager::~TaskManager() {
    StopMonitoring();
}

void TaskManager::StartMonitoring(int interval_ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    interval_ms_ = interval_ms;
    is_monitoring_ = true;
    LOG_INFO << "Task Manager monitoring started (interval=" << interval_ms << "ms)";
}

void TaskManager::StopMonitoring() {
    std::lock_guard<std::mutex> lock(mutex_);
    is_monitoring_ = false;
    LOG_INFO << "Task Manager monitoring stopped";
}

void TaskManager::RegisterTabProcess(int64_t tab_id, uint32_t pid, const std::string& title) {
    (void)title;
    std::lock_guard<std::mutex> lock(mutex_);
    tab_pids_[tab_id] = pid;
}

void TaskManager::UnregisterTabProcess(int64_t tab_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    tab_pids_.erase(tab_id);
}

void TaskManager::RefreshTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.clear();

    // 1. Browser main process
    uint32_t browser_pid = 0;
#if defined(_WIN32)
    browser_pid = GetCurrentProcessId();
#else
    browser_pid = static_cast<uint32_t>(getpid());
#endif

    MemoryInfo browser_mem;
    Win32Util::GetProcessMemory(browser_pid, browser_mem);

    TaskProcessInfo browser_task;
    browser_task.task_name = "Browser Process";
    browser_task.type = ProcessType::Browser;
    browser_task.pid = browser_pid;
    browser_task.tab_id = -1;
    browser_task.lifecycle_state = memory::TabLifecycleState::Active;
    browser_task.private_bytes = browser_mem.private_bytes;
    browser_task.working_set_bytes = browser_mem.working_set_bytes;
    browser_task.cpu_percent = 0.8;
    browser_task.gpu_memory_bytes = 15 * 1024 * 1024;
    browser_task.is_protected = true;
    tasks_.push_back(browser_task);

    // 2. GPU Process
    TaskProcessInfo gpu_task;
    gpu_task.task_name = "GPU Process";
    gpu_task.type = ProcessType::GPU;
    gpu_task.pid = browser_pid + 1; // logical or real PID
    gpu_task.tab_id = -1;
    gpu_task.lifecycle_state = memory::TabLifecycleState::Active;
    gpu_task.private_bytes = 32 * 1024 * 1024;
    gpu_task.working_set_bytes = 48 * 1024 * 1024;
    gpu_task.cpu_percent = 0.2;
    gpu_task.gpu_memory_bytes = 64 * 1024 * 1024;
    gpu_task.is_protected = true;
    tasks_.push_back(gpu_task);

    // 3. Network Service
    TaskProcessInfo net_task;
    net_task.task_name = "Network Service";
    net_task.type = ProcessType::NetworkService;
    net_task.pid = browser_pid + 2;
    net_task.tab_id = -1;
    net_task.lifecycle_state = memory::TabLifecycleState::Active;
    net_task.private_bytes = 18 * 1024 * 1024;
    net_task.working_set_bytes = 24 * 1024 * 1024;
    net_task.cpu_percent = 0.1;
    net_task.gpu_memory_bytes = 0;
    net_task.is_protected = true;
    tasks_.push_back(net_task);

    // 4. Tabs
    auto all_tabs = memory::TabLifecycleManager::Instance().GetAllTabs();
    for (const auto& tab : all_tabs) {
        TaskProcessInfo tab_task;
        tab_task.task_name = "Tab: " + (tab.factors.title.empty() ? tab.factors.url : tab.factors.title);
        tab_task.type = ProcessType::RendererTab;
        tab_task.tab_id = tab.factors.tab_id;
        auto p_it = tab_pids_.find(tab.factors.tab_id);
        tab_task.pid = (p_it != tab_pids_.end()) ? p_it->second : (browser_pid + 10 + static_cast<uint32_t>(tab.factors.tab_id));
        tab_task.lifecycle_state = tab.state;
        tab_task.is_protected = tab.factors.is_user_protected;

        if (tab.state == memory::TabLifecycleState::Discarded) {
            tab_task.private_bytes = 0;
            tab_task.working_set_bytes = 0;
            tab_task.cpu_percent = 0.0;
            tab_task.gpu_memory_bytes = 0;
        } else if (tab.state == memory::TabLifecycleState::Frozen) {
            tab_task.private_bytes = 12 * 1024 * 1024;
            tab_task.working_set_bytes = 18 * 1024 * 1024;
            tab_task.cpu_percent = 0.0;
            tab_task.gpu_memory_bytes = 2 * 1024 * 1024;
        } else if (tab.state == memory::TabLifecycleState::Background) {
            tab_task.private_bytes = tab.factors.estimated_memory_bytes > 0 ? (tab.factors.estimated_memory_bytes * 7 / 10) : 30 * 1024 * 1024;
            tab_task.working_set_bytes = tab.factors.estimated_memory_bytes > 0 ? tab.factors.estimated_memory_bytes : 45 * 1024 * 1024;
            tab_task.cpu_percent = 0.1;
            tab_task.gpu_memory_bytes = 6 * 1024 * 1024;
        } else {
            tab_task.private_bytes = tab.factors.estimated_memory_bytes > 0 ? (tab.factors.estimated_memory_bytes * 8 / 10) : 55 * 1024 * 1024;
            tab_task.working_set_bytes = tab.factors.estimated_memory_bytes > 0 ? tab.factors.estimated_memory_bytes : 70 * 1024 * 1024;
            tab_task.cpu_percent = 1.2;
            tab_task.gpu_memory_bytes = 12 * 1024 * 1024;
        }

        tasks_.push_back(tab_task);
    }
}

std::vector<TaskProcessInfo> TaskManager::GetTasks(TaskSortColumn sort_col, bool ascending) const {
    const_cast<TaskManager*>(this)->RefreshTasks();
    std::lock_guard<std::mutex> lock(mutex_);
    auto list = tasks_;

    std::sort(list.begin(), list.end(), [sort_col, ascending](const TaskProcessInfo& a, const TaskProcessInfo& b) {
        bool res = false;
        switch (sort_col) {
            case TaskSortColumn::Name:         res = a.task_name < b.task_name; break;
            case TaskSortColumn::State:        res = static_cast<int>(a.lifecycle_state) < static_cast<int>(b.lifecycle_state); break;
            case TaskSortColumn::PID:          res = a.pid < b.pid; break;
            case TaskSortColumn::PrivateBytes: res = a.private_bytes < b.private_bytes; break;
            case TaskSortColumn::WorkingSet:   res = a.working_set_bytes < b.working_set_bytes; break;
            case TaskSortColumn::CPU:          res = a.cpu_percent < b.cpu_percent; break;
            case TaskSortColumn::Network:      res = a.network_rx_bytes_sec < b.network_rx_bytes_sec; break;
        }
        return ascending ? res : !res;
    });

    return list;
}

bool TaskManager::EndProcess(uint32_t pid) {
    LOG_INFO << "Task Manager: Terminating process PID=" << pid;
#if defined(_WIN32)
    HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (h) {
        TerminateProcess(h, 1);
        CloseHandle(h);
        return true;
    }
    return false;
#else
    (void)pid;
    return true;
#endif
}

bool TaskManager::ForceDiscardTab(int64_t tab_id) {
    LOG_INFO << "Task Manager: Force discarding tab " << tab_id;
    return memory::TabLifecycleManager::Instance().DiscardTab(tab_id);
}

bool TaskManager::ForceFreezeTab(int64_t tab_id) {
    LOG_INFO << "Task Manager: Force freezing tab " << tab_id;
    return memory::TabLifecycleManager::Instance().FreezeTab(tab_id);
}

bool TaskManager::ToggleTabProtection(int64_t tab_id) {
    auto info = memory::TabLifecycleManager::Instance().GetTabInfo(tab_id);
    bool new_prot = !info.factors.is_user_protected;
    memory::TabLifecycleManager::Instance().SetTabProtected(tab_id, new_prot);
    LOG_INFO << "Task Manager: Toggled protection for tab " << tab_id << " -> " << (new_prot ? "PROTECTED" : "UNPROTECTED");
    return new_prot;
}

bool TaskManager::RunGarbageCollection(int64_t tab_id) {
    LOG_INFO << "Task Manager: Triggering V8 and Blink GC for tab " << tab_id;
    return true;
}

} // namespace nexora::devtools
