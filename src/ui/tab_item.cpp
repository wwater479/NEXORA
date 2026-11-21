#include "tab_item.h"

namespace nexora::ui {

TabItem::TabItem(int64_t id, const std::string& title, const std::string& url)
    : id_(id), title_(title), url_(url) {}

Rect TabItem::GetCloseButtonBounds() const {
    if (is_pinned_) return Rect{0, 0, 0, 0};
    int btn_size = 16;
    int margin_right = 8;
    int btn_x = bounds_.x + bounds_.width - margin_right - btn_size;
    int btn_y = bounds_.y + (bounds_.height - btn_size) / 2;
    return Rect{btn_x, btn_y, btn_size, btn_size};
}

Rect TabItem::GetAudioButtonBounds() const {
    if (!is_audio_playing_) return Rect{0, 0, 0, 0};
    int btn_size = 16;
    int margin_right = is_pinned_ ? 4 : 28;
    int btn_x = bounds_.x + bounds_.width - margin_right - btn_size;
    int btn_y = bounds_.y + (bounds_.height - btn_size) / 2;
    return Rect{btn_x, btn_y, btn_size, btn_size};
}

Rect TabItem::GetProtectionIconBounds() const {
    if (!is_protected_) return Rect{0, 0, 0, 0};
    int icon_size = 14;
    int icon_x = bounds_.x + (is_pinned_ ? 2 : 24);
    int icon_y = bounds_.y + (bounds_.height - icon_size) / 2;
    return Rect{icon_x, icon_y, icon_size, icon_size};
}

} // namespace nexora::ui
