#pragma once

#include <string>
#include <vector>
#include <functional>

namespace nexora::core {

enum class UpdateChannel {
    Stable,
    Beta,
    Dev
};

struct VersionInfo {
    std::string browser_name = "NEXORA";
    std::string browser_version = "1.0.0-alpha";
    std::string chromium_version = "120.0.6099.129";
    std::string cef_version = "120.1.10+g38cf81d+chromium-120.0.6099.129";
    std::string os_version = "Windows 11 x64";
    std::string build_date = __DATE__;
    std::string architecture = "x64";
};

class UpdateManager {
public:
    static UpdateManager& Instance() {
        static UpdateManager instance;
        return instance;
    }

    VersionInfo GetVersionInfo() const { return version_info_; }

    UpdateChannel GetChannel() const { return channel_; }
    void SetChannel(UpdateChannel channel) { channel_ = channel; }

    bool CheckForUpdates(std::function<void(bool update_available, const std::string& new_version, const std::string& release_notes)> callback);

private:
    UpdateManager() = default;
    ~UpdateManager() = default;

    VersionInfo version_info_;
    UpdateChannel channel_ = UpdateChannel::Stable;
};

} // namespace nexora::core
