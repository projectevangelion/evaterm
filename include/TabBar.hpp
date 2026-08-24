#pragma once

#include "TabManager.hpp"
#include "Theme.hpp"
#include <SDL2/SDL_opengl.h>
#include <string>
#include <vector>

namespace evaterm {

struct Rect {
    float x = 0;
    float y = 0;
    float w = 0;
    float h = 0;

    bool contains(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

struct TabItemLayout {
    Rect bounds;
    Rect close_button_bounds;
    std::string title;
    bool is_active = false;
};

class TabBar {
public:
    TabBar(int height = 30);

    void update_layout(const TabManager& tab_manager, int window_width);

    int get_tab_at_point(float x, float y) const;
    int get_close_button_at_point(float x, float y) const;
    bool is_add_button_at_point(float x, float y) const;

    int get_height() const { return height_; }
    void set_height(int h) { height_ = h; }

    const std::vector<TabItemLayout>& get_layouts() const { return tab_layouts_; }
    const Rect& get_add_button_bounds() const { return add_button_bounds_; }

    void set_hover_state(int hovered_tab, int hovered_close, bool hovered_add) {
        hovered_tab_ = hovered_tab;
        hovered_close_ = hovered_close;
        hovered_add_ = hovered_add;
    }

    int get_hovered_tab() const { return hovered_tab_; }
    int get_hovered_close() const { return hovered_close_; }
    bool get_hovered_add() const { return hovered_add_; }

private:
    int height_ = 30;
    std::vector<TabItemLayout> tab_layouts_;
    Rect add_button_bounds_;

    int hovered_tab_ = -1;
    int hovered_close_ = -1;
    bool hovered_add_ = false;
};

} // namespace evaterm
