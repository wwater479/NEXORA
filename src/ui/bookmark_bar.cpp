#include "bookmark_bar.h"

namespace nexora::ui {

BookmarkBar::BookmarkBar() {}

void BookmarkBar::Refresh(const std::shared_ptr<storage::BookmarkNode>& bookmark_bar_node) {
    buttons_.clear();
    if (!bookmark_bar_node) return;

    for (const auto& child : bookmark_bar_node->children) {
        BookmarkButton btn;
        btn.id = child->id;
        btn.title = child->title;
        btn.url = child->url;
        btn.is_folder = (child->type == storage::BookmarkNodeType::Folder);
        buttons_.push_back(btn);
    }
}

void BookmarkBar::UpdateLayout(int bar_x, int bar_y, int bar_width, int bar_height) {
    (void)bar_width;
    int cur_x = bar_x + 8;
    int btn_height = bar_height - 6;
    int btn_y = bar_y + 3;

    for (auto& btn : buttons_) {
        int btn_width = std::max(60, std::min(160, static_cast<int>(btn.title.length() * 8 + 24)));
        btn.bounds = Rect{cur_x, btn_y, btn_width, btn_height};
        cur_x += btn_width + 6;
    }
}

int BookmarkBar::HitTest(int px, int py) const {
    if (!is_visible_) return -1;
    for (size_t i = 0; i < buttons_.size(); ++i) {
        if (buttons_[i].bounds.Contains(px, py)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void BookmarkBar::OnClick(int button_index) {
    if (button_index >= 0 && button_index < static_cast<int>(buttons_.size())) {
        if (!buttons_[button_index].is_folder && click_cb_) {
            click_cb_(buttons_[button_index].url);
        }
    }
}

} // namespace nexora::ui
