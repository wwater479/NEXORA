#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace nexora::ui {

class TabGroup {
public:
    TabGroup(int group_id, const std::string& title, const std::string& color_hex = "#1a73e8");

    int GetId() const { return group_id_; }

    std::string GetTitle() const { return title_; }
    void SetTitle(const std::string& title) { title_ = title; }

    std::string GetColorHex() const { return color_hex_; }
    void SetColorHex(const std::string& color) { color_hex_ = color; }

    bool IsCollapsed() const { return is_collapsed_; }
    void SetCollapsed(bool collapsed) { is_collapsed_ = collapsed; }

    void AddTab(int64_t tab_id);
    void RemoveTab(int64_t tab_id);
    bool HasTab(int64_t tab_id) const;

    const std::vector<int64_t>& GetTabs() const { return tab_ids_; }

private:
    int group_id_;
    std::string title_;
    std::string color_hex_;
    bool is_collapsed_ = false;
    std::vector<int64_t> tab_ids_;
};

} // namespace nexora::ui
