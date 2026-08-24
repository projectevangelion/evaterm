#pragma once

#include "FontAtlas.hpp"
#include "ScreenBuffer.hpp"
#include "Theme.hpp"
#include "TabBar.hpp"
#include "Config.hpp"
#include <SDL2/SDL_opengl.h>
#include <memory>
#include <string>

namespace evaterm {

class Renderer {
public:
    Renderer(FontAtlas& font_atlas, const Config& config);
    ~Renderer() = default;

    void init();
    void render(const TabManager& tab_manager, const TabBar& tab_bar, int window_width, int window_height, bool window_focused);

    void draw_quad(float x, float y, float w, float h, const Color& color);
    void draw_text(float x, float y, const std::string& text, const Color& color, int max_width = -1);
    void draw_char(float x, float y, char32_t codepoint, const Color& color);

    void update_config(const Config& config) { config_ = config; }

private:
    FontAtlas& font_atlas_;
    Config config_;

    void render_tab_bar(const TabBar& tab_bar, int window_width);
    void render_terminal_grid(const ScreenBuffer& buffer, int grid_top, int grid_left, int window_width, int window_height, bool window_focused);
    void render_cursor(const ScreenBuffer& buffer, int grid_top, int grid_left, bool window_focused);
    void render_scroll_indicator(const ScreenBuffer& buffer, int grid_top, int window_width, int window_height);
};

} // namespace evaterm
