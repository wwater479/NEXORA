#pragma once

#include "tab_item.h"
#include "../storage/bookmark_store.h"
#include <vector>
#include <memory>
#include <functional>

namespace nexora::ui {

struct BookmarkButton {
    int64_t id = 0;
    std::string title;
    std::string url;
    bool is_folder = false;
    Rect bounds;
};

class BookmarkBar {
public:
    using BookmarkClickCallback = std::function<void(const std::string& url)>;

    BookmarkBar();
    ~BookmarkBar() = default;

    void SetVisible(bool visible) { is_visible_ = visible; }
    bool IsVisible() const { return is_visible_; }

    void Refresh(const std::shared_ptr<storage::BookmarkNode>& bookmark_bar_node);
    void UpdateLayout(int bar_x, int bar_y, int bar_width, int bar_height);

    const std::vector<BookmarkButton>& GetButtons() const { return buttons_; }
    int HitTest(int px, int py) const;

    void SetCallback(BookmarkClickCallback cb) { click_cb_ = cb; }
    void OnClick(int button_index);

private:
    bool is_visible_ = true;
    std::vector<BookmarkButton> buttons_;
    BookmarkClickCallback click_cb_;
};

} // namespace nexora::ui
