#include "download_ui.h"

namespace nexora::ui {

DownloadUI::DownloadUI() {}

void DownloadUI::UpdateDownloads(const std::vector<storage::DownloadItem>& downloads) {
    downloads_ = downloads;
    is_visible_ = !downloads_.empty();
}

void DownloadUI::UpdateLayout(int win_x, int win_y, int win_width, int win_height) {
    int bar_height = 48;
    bounds_ = Rect{win_x, win_y + win_height - bar_height, win_width, bar_height};
}

} // namespace nexora::ui
