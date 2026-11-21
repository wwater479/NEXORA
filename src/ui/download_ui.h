#pragma once

#include "tab_item.h"
#include "../storage/download_store.h"
#include <vector>
#include <functional>

namespace nexora::ui {

class DownloadUI {
public:
    using DownloadActionCallback = std::function<void(int64_t download_id, const std::string& action)>;

    DownloadUI();
    ~DownloadUI() = default;

    void SetVisible(bool visible) { is_visible_ = visible; }
    bool IsVisible() const { return is_visible_; }

    void UpdateDownloads(const std::vector<storage::DownloadItem>& downloads);
    void UpdateLayout(int win_x, int win_y, int win_width, int win_height);

    Rect GetBounds() const { return bounds_; }
    const std::vector<storage::DownloadItem>& GetDownloads() const { return downloads_; }

    void SetCallback(DownloadActionCallback cb) { action_cb_ = cb; }

private:
    bool is_visible_ = false;
    Rect bounds_;
    std::vector<storage::DownloadItem> downloads_;
    DownloadActionCallback action_cb_;
};

} // namespace nexora::ui
