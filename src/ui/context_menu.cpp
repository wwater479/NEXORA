#include "context_menu.h"

namespace nexora::ui {

ContextMenu::ContextMenu() {}

void ContextMenu::AddItem(int id, const std::string& text, std::function<void()> action, bool enabled) {
    MenuItem item;
    item.id = id;
    item.text = text;
    item.type = MenuItemType::Normal;
    item.is_enabled = enabled;
    item.action = action;
    items_.push_back(item);
}

void ContextMenu::AddCheckbox(int id, const std::string& text, bool is_checked, std::function<void()> action, bool enabled) {
    MenuItem item;
    item.id = id;
    item.text = text;
    item.type = MenuItemType::Checkbox;
    item.is_checked = is_checked;
    item.is_enabled = enabled;
    item.action = action;
    items_.push_back(item);
}

void ContextMenu::AddSeparator() {
    MenuItem item;
    item.type = MenuItemType::Separator;
    items_.push_back(item);
}

void ContextMenu::ExecuteItem(int id) {
    for (const auto& item : items_) {
        if (item.id == id && item.is_enabled && item.action) {
            item.action();
            break;
        }
    }
}

ContextMenu ContextMenu::CreateTabContextMenu(
    int64_t tab_id, bool is_pinned, bool is_muted, bool is_protected,
    std::function<void(int64_t, const std::string&)> action_handler) {

    ContextMenu menu;
    menu.AddItem(1, "New Tab to the Right", [=]() { if (action_handler) action_handler(tab_id, "new_tab"); });
    menu.AddItem(2, "Reload", [=]() { if (action_handler) action_handler(tab_id, "reload"); });
    menu.AddItem(3, "Duplicate", [=]() { if (action_handler) action_handler(tab_id, "duplicate"); });
    menu.AddItem(4, is_pinned ? "Unpin Tab" : "Pin Tab", [=]() { if (action_handler) action_handler(tab_id, "toggle_pin"); });
    menu.AddItem(5, is_muted ? "Unmute Tab" : "Mute Tab", [=]() { if (action_handler) action_handler(tab_id, "toggle_mute"); });
    menu.AddSeparator();
    menu.AddItem(6, "Freeze Tab Now", [=]() { if (action_handler) action_handler(tab_id, "freeze"); });
    menu.AddItem(7, "Discard Tab Now (Save RAM)", [=]() { if (action_handler) action_handler(tab_id, "discard"); });
    menu.AddCheckbox(8, "Protect from Auto-Discard", is_protected, [=]() { if (action_handler) action_handler(tab_id, "toggle_protect"); });
    menu.AddSeparator();
    menu.AddItem(9, "Close Tab", [=]() { if (action_handler) action_handler(tab_id, "close"); });
    menu.AddItem(10, "Close Other Tabs", [=]() { if (action_handler) action_handler(tab_id, "close_others"); });
    menu.AddItem(11, "Close Tabs to the Right", [=]() { if (action_handler) action_handler(tab_id, "close_right"); });
    return menu;
}

ContextMenu ContextMenu::CreatePageContextMenu(
    bool can_back, bool can_fwd,
    std::function<void(const std::string&)> action_handler) {

    ContextMenu menu;
    menu.AddItem(101, "Back", [=]() { if (action_handler) action_handler("back"); }, can_back);
    menu.AddItem(102, "Forward", [=]() { if (action_handler) action_handler("forward"); }, can_fwd);
    menu.AddItem(103, "Reload", [=]() { if (action_handler) action_handler("reload"); });
    menu.AddSeparator();
    menu.AddItem(104, "Save Page As...", [=]() { if (action_handler) action_handler("save_as"); });
    menu.AddItem(105, "Print...", [=]() { if (action_handler) action_handler("print"); });
    menu.AddSeparator();
    menu.AddItem(106, "View Page Source", [=]() { if (action_handler) action_handler("view_source"); });
    menu.AddItem(107, "Inspect Element (DevTools)", [=]() { if (action_handler) action_handler("inspect"); });
    return menu;
}

} // namespace nexora::ui
