#pragma once

#include "Config.hpp"
#include "FontAtlas.hpp"
#include "TabManager.hpp"
#include "TabBar.hpp"
#include "Renderer.hpp"
#include "InputHandler.hpp"
#include <SDL2/SDL.h>
#include <string>
#include <memory>

namespace evaterm {

class Window {
public:
    Window(Config config);
    ~Window();

    bool init();
    void run();

private:
    Config config_;
    SDL_Window* sdl_window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;

    int window_width_ = 960;
    int window_height_ = 600;
    bool is_running_ = false;
    bool is_focused_ = true;

    FontAtlas font_atlas_;
    std::unique_ptr<TabManager> tab_manager_;
    std::unique_ptr<TabBar> tab_bar_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<InputHandler> input_handler_;

    int inotify_fd_ = -1;
    int inotify_wd_ = -1;

    void handle_resize(int new_width, int new_height);
    void update_grid_dimensions();
    void update_window_title();
    void reload_config();
    void set_font_size(int new_size);
    void init_config_watcher();
    void check_config_watcher();
};

} // namespace evaterm
