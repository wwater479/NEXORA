#include "theme_manager.h"

namespace nexora::ui {

ThemeManager::ThemeManager() {
    SetDarkMode(true);
}

void ThemeManager::SetDarkMode(bool dark) {
    colors_.is_dark = dark;
    if (dark) {
        colors_.background = 0x202124;
        colors_.foreground = 0xE8EAED;
        colors_.tab_bar_bg = 0x1A1B1E;
        colors_.tab_active_bg = 0x323639;
        colors_.tab_inactive_bg = 0x202124;
        colors_.tab_hover_bg = 0x292A2D;
        colors_.tab_text = 0xE8EAED;
        colors_.tab_text_inactive = 0x9AA0A6;
        colors_.address_bar_bg = 0x28292C;
        colors_.address_bar_focus_bg = 0x303134;
        colors_.address_bar_border = 0x3C4043;
        colors_.address_bar_text = 0xFFFFFF;
        colors_.accent = 0x8AB4F8;
    } else {
        colors_.background = 0xF1F3F4;
        colors_.foreground = 0x202124;
        colors_.tab_bar_bg = 0xDEE1E6;
        colors_.tab_active_bg = 0xFFFFFF;
        colors_.tab_inactive_bg = 0xDEE1E6;
        colors_.tab_hover_bg = 0xE8EAED;
        colors_.tab_text = 0x202124;
        colors_.tab_text_inactive = 0x5F6368;
        colors_.address_bar_bg = 0xF1F3F4;
        colors_.address_bar_focus_bg = 0xFFFFFF;
        colors_.address_bar_border = 0xDADCE0;
        colors_.address_bar_text = 0x202124;
        colors_.accent = 0x1A73E8;
    }
}

} // namespace nexora::ui
