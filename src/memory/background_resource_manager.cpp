#include "background_resource_manager.h"

namespace nexora::memory {

void BackgroundResourceManager::SetTabThrottled(int64_t tab_id, bool throttled) {
    std::lock_guard<std::mutex> lock(mutex_);
    throttled_tabs_[tab_id] = throttled;
}

bool BackgroundResourceManager::IsTabThrottled(int64_t tab_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = throttled_tabs_.find(tab_id);
    if (it != throttled_tabs_.end()) return it->second;
    return false;
}

void BackgroundResourceManager::RegisterAudioStream(int64_t tab_id, bool is_playing) {
    std::lock_guard<std::mutex> lock(mutex_);
    audio_streams_[tab_id] = is_playing;
}

bool BackgroundResourceManager::IsAudioStreamPlaying(int64_t tab_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = audio_streams_.find(tab_id);
    if (it != audio_streams_.end()) return it->second;
    return false;
}

void BackgroundResourceManager::RegisterMediaSession(int64_t tab_id, const std::string& title, const std::string& artist) {
    std::lock_guard<std::mutex> lock(mutex_);
    media_sessions_[tab_id] = {title, artist};
}

void BackgroundResourceManager::UnregisterMediaSession(int64_t tab_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    media_sessions_.erase(tab_id);
}

} // namespace nexora::memory
