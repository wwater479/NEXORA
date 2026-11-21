#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace nexora::ui {

enum class MenuItemType {
    Normal,
    Separator,
    Checkbox
};

struct MenuItem {
    int id = 0;
    std::string text;
    MenuItemType type = MenuItemType::Normal;
    bool is_checked = false;
    bool is_enabled = true;
    std::function<void()> action;
};

class ContextMenu {
public:
    ContextMenu();
    ~ContextMenu() = default;

    void AddItem(int id, const std::string& text, std::function<void()> action, bool enabled = true);
    void AddCheckbox(int id, const std::string& text, bool is_checked, std::function<void()> action, bool enabled = true);
    void AddSeparator();

    const std::vector<MenuItem>& GetItems() const { return items_; }

    void ExecuteItem(int id);

    // Factory helpers
    static ContextMenu CreateTabContextMenu(
        int64_t tab_id, bool is_pinned, bool is_muted, bool is_protected,
        std::function<void(int64_t, const std::string&)> action_handler);

    static ContextMenu CreatePageContextMenu(
        bool can_back, bool can_fwd,
        std::function<void(const std::string&)> action_handler);

private:
    std::vector<MenuItem> items_;
};

} // namespace nexora::ui
