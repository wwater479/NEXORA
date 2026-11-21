#include "navigation_controls.h"

namespace nexora::ui {

NavigationControls::NavigationControls() {}

void NavigationControls::UpdateLayout(int start_x, int start_y, int height) {
    int btn_size = 28;
    int margin = 4;
    int cur_x = start_x;
    int btn_y = start_y + (height - btn_size) / 2;

    back_bounds_ = Rect{cur_x, btn_y, btn_size, btn_size};
    cur_x += btn_size + margin;

    forward_bounds_ = Rect{cur_x, btn_y, btn_size, btn_size};
    cur_x += btn_size + margin;

    reload_bounds_ = Rect{cur_x, btn_y, btn_size, btn_size};
    cur_x += btn_size + margin;

    home_bounds_ = Rect{cur_x, btn_y, btn_size, btn_size};
    cur_x += btn_size + margin;

    total_width_ = cur_x - start_x;
}

} // namespace nexora::ui
