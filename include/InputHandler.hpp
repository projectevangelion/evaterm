#pragma once

#include "TabManager.hpp"
#include "FontAtlas.hpp"
#include "Config.hpp"
#include "TabBar.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <functional>

namespace evaterm {

class InputHandler {
public:
    using ConfigReloadCallback = std::function<void()>;
    using FontResizeCallback = std::function<void(int)>;

    InputHandler(TabManager& tab_manager, FontAtlas& font_atlas, Config& config, TabBar& tab_bar);

    bool handle_event(const SDL_Event& event, int window_width, int window_height);

    void set_on_config_reload(ConfigReloadCallback cb) { on_config_reload_ = cb; }
    void set_on_font_resize(FontResizeCallback cb) { on_font_resize_ = cb; }

private:
    TabManager& tab_manager_;
    FontAtlas& font_atlas_;
    Config& config_;
    TabBar& tab_bar_;

    ConfigReloadCallback on_config_reload_;
    FontResizeCallback on_font_resize_;

    bool selecting_ = false;

    bool handle_keydown(const SDL_KeyboardEvent& key);
    bool handle_text_input(const SDL_TextInputEvent& text);
    bool handle_mouse_button(const SDL_MouseButtonEvent& button, int win_w, int win_h);
    bool handle_mouse_motion(const SDL_MouseMotionEvent& motion, int win_w, int win_h);
    bool handle_mouse_wheel(const SDL_MouseWheelEvent& wheel);

    void paste_from_clipboard();
    void copy_to_clipboard();
};

} // namespace evaterm
