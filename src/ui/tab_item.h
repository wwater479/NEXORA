#pragma once

#include "../memory/memory_types.h"
#include <string>
#include <cstdint>

namespace nexora::ui {

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool Contains(int px, int py) const {
        return px >= x && px <= (x + width) && py >= y && py <= (y + height);
    }
};

class TabItem {
public:
    TabItem(int64_t id, const std::string& title, const std::string& url);

    int64_t GetId() const { return id_; }

    std::string GetTitle() const { return title_; }
    void SetTitle(const std::string& title) { title_ = title; }

    std::string GetUrl() const { return url_; }
    void SetUrl(const std::string& url) { url_ = url; }

    std::string GetFaviconUrl() const { return favicon_url_; }
    void SetFaviconUrl(const std::string& favicon) { favicon_url_ = favicon; }

    bool IsActive() const { return is_active_; }
    void SetActive(bool active) { is_active_ = active; }

    bool IsPinned() const { return is_pinned_; }
    void SetPinned(bool pinned) { is_pinned_ = pinned; }

    bool IsAudioPlaying() const { return is_audio_playing_; }
    void SetAudioPlaying(bool playing) { is_audio_playing_ = playing; }

    bool IsMuted() const { return is_muted_; }
    void SetMuted(bool muted) { is_muted_ = muted; }

    bool IsLoading() const { return is_loading_; }
    void SetLoading(bool loading) { is_loading_ = loading; }

    bool IsProtected() const { return is_protected_; }
    void SetProtected(bool is_prot) { is_protected_ = is_prot; }

    memory::TabLifecycleState GetLifecycleState() const { return lifecycle_state_; }
    void SetLifecycleState(memory::TabLifecycleState state) { lifecycle_state_ = state; }

    int GetGroupId() const { return group_id_; }
    void SetGroupId(int group_id) { group_id_ = group_id; }

    Rect GetBounds() const { return bounds_; }
    void SetBounds(const Rect& bounds) { bounds_ = bounds; }

    Rect GetCloseButtonBounds() const;
    Rect GetAudioButtonBounds() const;
    Rect GetProtectionIconBounds() const;

private:
    int64_t id_;
    std::string title_;
    std::string url_;
    std::string favicon_url_;
    bool is_active_ = false;
    bool is_pinned_ = false;
    bool is_audio_playing_ = false;
    bool is_muted_ = false;
    bool is_loading_ = false;
    bool is_protected_ = false;
    memory::TabLifecycleState lifecycle_state_ = memory::TabLifecycleState::Active;
    int group_id_ = -1;
    Rect bounds_;
};

} // namespace nexora::ui
