#pragma once

#include <string>
#include <cstdint>

namespace nexora::ui {

struct ThemeColors {
    uint32_t background = 0x202124;
    uint32_t foreground = 0xE8EAED;
    uint32_t tab_bar_bg = 0x1A1B1E;
    uint32_t tab_active_bg = 0x323639;
    uint32_t tab_inactive_bg = 0x202124;
    uint32_t tab_hover_bg = 0x292A2D;
    uint32_t tab_text = 0xE8EAED;
    uint32_t tab_text_inactive = 0x9AA0A6;
    uint32_t tab_close_hover = 0x5F6368;
    uint32_t address_bar_bg = 0x28292C;
    uint32_t address_bar_focus_bg = 0x303134;
    uint32_t address_bar_border = 0x3C4043;
    uint32_t address_bar_text = 0xFFFFFF;
    uint32_t button_hover = 0x3C4043;
    uint32_t separator = 0x3C4043;
    uint32_t accent = 0x8AB4F8;
    uint32_t memory_badge_active = 0x1E8E3E;
    uint32_t memory_badge_frozen = 0x1A73E8;
    uint32_t memory_badge_discarded = 0xD93025;
    bool is_dark = true;
};

class ThemeManager {
public:
    static ThemeManager& Instance() {
        static ThemeManager instance;
        return instance;
    }

    void SetDarkMode(bool dark);
    bool IsDarkMode() const { return colors_.is_dark; }

    ThemeColors GetColors() const { return colors_; }

private:
    ThemeManager();
    ~ThemeManager() = default;

    ThemeColors colors_;
};

} // namespace nexora::ui
