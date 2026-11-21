#pragma once

#include <string>
#include <map>
#include <mutex>
#include <cstdint>

namespace nexora::devtools {

class DevToolsManager {
public:
    static DevToolsManager& Instance() {
        static DevToolsManager instance;
        return instance;
    }

    void Init(bool remote_debugging_enabled = false, int remote_port = 9222);

    void OpenDevToolsForTab(int64_t tab_id, void* browser_host_ptr = nullptr);
    void CloseDevToolsForTab(int64_t tab_id);
    bool IsDevToolsOpenForTab(int64_t tab_id) const;

    bool IsRemoteDebuggingEnabled() const { return remote_debugging_enabled_; }
    int GetRemoteDebuggingPort() const { return remote_port_; }
    std::string GetRemoteDebuggingUrl() const;

private:
    DevToolsManager() = default;
    ~DevToolsManager() = default;

    mutable std::mutex mutex_;
    bool remote_debugging_enabled_ = false;
    int remote_port_ = 9222;
    std::map<int64_t, bool> active_devtools_tabs_;
};

} // namespace nexora::devtools
