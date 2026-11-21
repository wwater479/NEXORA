#include "devtools_manager.h"
#include "../util/logger.h"

namespace nexora::devtools {

void DevToolsManager::Init(bool remote_debugging_enabled, int remote_port) {
    std::lock_guard<std::mutex> lock(mutex_);
    remote_debugging_enabled_ = remote_debugging_enabled;
    remote_port_ = remote_port;
    if (remote_debugging_enabled_) {
        LOG_INFO << "DevTools Remote Debugging active at port " << remote_port_;
    }
}

void DevToolsManager::OpenDevToolsForTab(int64_t tab_id, void* browser_host_ptr) {
    (void)browser_host_ptr;
    std::lock_guard<std::mutex> lock(mutex_);
    active_devtools_tabs_[tab_id] = true;
    LOG_INFO << "DevTools opened for tab " << tab_id;
}

void DevToolsManager::CloseDevToolsForTab(int64_t tab_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    active_devtools_tabs_.erase(tab_id);
    LOG_INFO << "DevTools closed for tab " << tab_id;
}

bool DevToolsManager::IsDevToolsOpenForTab(int64_t tab_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = active_devtools_tabs_.find(tab_id);
    return it != active_devtools_tabs_.end() && it->second;
}

std::string DevToolsManager::GetRemoteDebuggingUrl() const {
    return "http://127.0.0.1:" + std::to_string(remote_port_);
}

} // namespace nexora::devtools
