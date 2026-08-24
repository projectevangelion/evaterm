#include "TabBar.hpp"
#include <algorithm>

namespace evaterm {

TabBar::TabBar(int height) : height_(height) {}

void TabBar::update_layout(const TabManager& tab_manager, int window_width) {
    tab_layouts_.clear();
    size_t count = tab_manager.get_tab_count();
    if (count == 0) return;

    float available_width = static_cast<float>(window_width - 40); // reserve space for '+' button
    float max_tab_width = 220.0f;
    float min_tab_width = 90.0f;
    float tab_width = std::clamp(available_width / count, min_tab_width, max_tab_width);

    float current_x = 0.0f;

    for (size_t i = 0; i < count; ++i) {
        auto tab = tab_manager.get_tab(i);
        TabItemLayout layout;
        layout.bounds = Rect{current_x, 0.0f, tab_width - 1.0f, static_cast<float>(height_)};
        layout.close_button_bounds = Rect{current_x + tab_width - 24.0f, (height_ - 16.0f) / 2.0f, 16.0f, 16.0f};
        layout.title = tab ? tab->get_title() : "Terminal";
        layout.is_active = (i == tab_manager.get_active_index());

        tab_layouts_.push_back(layout);
        current_x += tab_width;
    }

    add_button_bounds_ = Rect{current_x + 4.0f, (height_ - 22.0f) / 2.0f, 24.0f, 22.0f};
}

int TabBar::get_tab_at_point(float x, float y) const {
    if (y < 0 || y > height_) return -1;
    for (size_t i = 0; i < tab_layouts_.size(); ++i) {
        if (tab_layouts_[i].bounds.contains(x, y)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int TabBar::get_close_button_at_point(float x, float y) const {
    if (y < 0 || y > height_) return -1;
    for (size_t i = 0; i < tab_layouts_.size(); ++i) {
        if (tab_layouts_[i].close_button_bounds.contains(x, y)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool TabBar::is_add_button_at_point(float x, float y) const {
    return add_button_bounds_.contains(x, y);
}

} // namespace evaterm
