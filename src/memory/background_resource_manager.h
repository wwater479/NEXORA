#pragma once

#include <string>
#include <map>
#include <mutex>
#include <cstdint>

namespace nexora::memory {

class BackgroundResourceManager {
public:
    static BackgroundResourceManager& Instance() {
        static BackgroundResourceManager instance;
        return instance;
    }

    void SetTabThrottled(int64_t tab_id, bool throttled);
    bool IsTabThrottled(int64_t tab_id) const;

    void RegisterAudioStream(int64_t tab_id, bool is_playing);
    bool IsAudioStreamPlaying(int64_t tab_id) const;

    void RegisterMediaSession(int64_t tab_id, const std::string& title, const std::string& artist);
    void UnregisterMediaSession(int64_t tab_id);

private:
    BackgroundResourceManager() = default;
    ~BackgroundResourceManager() = default;

    mutable std::mutex mutex_;
    std::map<int64_t, bool> throttled_tabs_;
    std::map<int64_t, bool> audio_streams_;
    std::map<int64_t, std::pair<std::string, std::string>> media_sessions_;
};

} // namespace nexora::memory
