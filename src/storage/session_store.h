#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <cstdint>

namespace nexora::storage {

struct TabSessionData {
    int64_t id = 0;
    std::string url;
    std::string title;
    std::string favicon_url;
    bool is_pinned = false;
    bool is_muted = false;
    bool is_protected = false;
    std::string lifecycle_state = "ACTIVE"; // ACTIVE, BACKGROUND, FROZEN, DISCARDED
    int group_id = -1;
    int64_t last_active_timestamp = 0;
    int scroll_x = 0;
    int scroll_y = 0;
    std::vector<std::string> navigation_history;
    int current_history_index = -1;
};

struct TabGroupSessionData {
    int group_id = 0;
    std::string title;
    std::string color_hex;
    bool is_collapsed = false;
};

struct WindowSessionData {
    int window_id = 0;
    int x = 100;
    int y = 100;
    int width = 1200;
    int height = 800;
    bool is_maximized = false;
    int active_tab_index = 0;
    std::vector<TabSessionData> tabs;
    std::vector<TabGroupSessionData> groups;
};

struct BrowserSessionData {
    std::vector<WindowSessionData> windows;
    bool crashed = false;
    int64_t timestamp = 0;
};

class SessionStore {
public:
    explicit SessionStore(const std::string& session_file_path);
    ~SessionStore();

    bool LoadSession(BrowserSessionData& out_session);
    bool SaveSession(const BrowserSessionData& session);

    bool HasCrashSession() const;
    void ClearCrashFlag();

private:
    std::string session_file_path_;
    std::string backup_file_path_;
    mutable std::mutex mutex_;
};

} // namespace nexora::storage
