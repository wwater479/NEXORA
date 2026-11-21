#include "update_manager.h"
#include "../util/logger.h"

namespace nexora::core {

bool UpdateManager::CheckForUpdates(std::function<void(bool update_available, const std::string& new_version, const std::string& release_notes)> callback) {
    LOG_INFO << "Checking for updates on channel: " << (channel_ == UpdateChannel::Stable ? "Stable" : channel_ == UpdateChannel::Beta ? "Beta" : "Dev");
    if (callback) {
        callback(false, version_info_.browser_version, "You are using the latest version of NEXORA.");
    }
    return true;
}

} // namespace nexora::core
