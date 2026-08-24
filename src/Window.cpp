#include "Window.hpp"
#include <iostream>
#include <algorithm>
#include <sys/inotify.h>
#include <unistd.h>
#include <fcntl.h>

namespace evaterm {

Window::Window(Config config)
    : config_(std::move(config)) {}

Window::~Window() {
    if (inotify_wd_ >= 0 && inotify_fd_ >= 0) {
        inotify_rm_watch(inotify_fd_, inotify_wd_);
        inotify_wd_ = -1;
    }
    if (inotify_fd_ >= 0) {
        close(inotify_fd_);
        inotify_fd_ = -1;
    }

    if (gl_context_) {
        SDL_GL_DeleteContext(gl_context_);
        gl_context_ = nullptr;
    }
    if (sdl_window_) {
        SDL_DestroyWindow(sdl_window_);
        sdl_window_ = nullptr;
    }
    SDL_Quit();
}

bool Window::init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << "\n";
        return false;
    }

    // Configure OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    window_width_ = 960;
    window_height_ = 600;

    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;
    sdl_window_ = SDL_CreateWindow(
        "EvaTerm",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_width_,
        window_height_,
        window_flags
    );

    if (!sdl_window_) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << "\n";
        return false;
    }

    gl_context_ = SDL_GL_CreateContext(sdl_window_);
    if (!gl_context_) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << "\n";
        return false;
    }

    SDL_GL_MakeCurrent(sdl_window_, gl_context_);
    SDL_GL_SetSwapInterval(1); // Enable VSync

    // Initialize FontAtlas AFTER OpenGL context is active
    if (!font_atlas_.init(config_.font_family, config_.font_size)) {
        std::cerr << "Warning: Failed to load specified font: " << config_.font_family << ", falling back\n";
    }

    // Set initial window dimensions based on cell size
    int init_cols = 100;
    int init_rows = 30;
    window_width_ = init_cols * font_atlas_.get_cell_width() + 2 * config_.padding_x;
    window_height_ = init_rows * font_atlas_.get_cell_height() + config_.tab_bar_height + 2 * config_.padding_y;
    SDL_SetWindowSize(sdl_window_, window_width_, window_height_);
    SDL_SetWindowPosition(sdl_window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    // Initialize TabBar, Renderer, TabManager, InputHandler
    tab_bar_ = std::make_unique<TabBar>(config_.tab_bar_height);
    renderer_ = std::make_unique<Renderer>(font_atlas_, config_);
    renderer_->init();

    int initial_cols = std::max(20, (window_width_ - 2 * config_.padding_x) / font_atlas_.get_cell_width());
    int initial_rows = std::max(5, (window_height_ - config_.tab_bar_height - 2 * config_.padding_y) / font_atlas_.get_cell_height());

    tab_manager_ = std::make_unique<TabManager>(initial_rows, initial_cols, config_);
    input_handler_ = std::make_unique<InputHandler>(*tab_manager_, font_atlas_, config_, *tab_bar_);

    input_handler_->set_on_config_reload([this]() {
        reload_config();
    });

    input_handler_->set_on_font_resize([this](int new_size) {
        set_font_size(new_size);
    });

    init_config_watcher();
    SDL_StartTextInput();
    return true;
}

void Window::init_config_watcher() {
    inotify_fd_ = inotify_init1(IN_NONBLOCK);
    if (inotify_fd_ >= 0 && !config_.config_file_path.empty()) {
        inotify_wd_ = inotify_add_watch(inotify_fd_, config_.config_file_path.c_str(), IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO);
    }
}

void Window::check_config_watcher() {
    if (inotify_fd_ < 0) return;
    char buffer[4096];
    ssize_t length = read(inotify_fd_, buffer, sizeof(buffer));
    if (length > 0) {
        reload_config();
        if (inotify_wd_ >= 0) {
            inotify_rm_watch(inotify_fd_, inotify_wd_);
        }
        if (!config_.config_file_path.empty()) {
            inotify_wd_ = inotify_add_watch(inotify_fd_, config_.config_file_path.c_str(), IN_MODIFY | IN_CLOSE_WRITE | IN_MOVED_TO);
        }
    }
}

void Window::update_grid_dimensions() {
    if (!tab_manager_) return;

    bool show_tabs = config_.show_tab_bar_single_tab || tab_manager_->get_tab_count() > 1;
    int tab_bar_h = show_tabs ? tab_bar_->get_height() : 0;

    int avail_w = window_width_ - 2 * config_.padding_x;
    int avail_h = window_height_ - tab_bar_h - 2 * config_.padding_y;

    int cell_w = font_atlas_.get_cell_width();
    int cell_h = font_atlas_.get_cell_height();

    int cols = std::max(20, avail_w / cell_w);
    int rows = std::max(5, avail_h / cell_h);

    tab_manager_->resize_all(rows, cols);
}

void Window::handle_resize(int new_width, int new_height) {
    window_width_ = new_width;
    window_height_ = new_height;
    update_grid_dimensions();
}

void Window::set_font_size(int new_size) {
    if (new_size < 6 || new_size > 48) return;
    config_.font_size = new_size;
    font_atlas_.set_font_size(new_size);
    update_grid_dimensions();
}

void Window::reload_config() {
    std::cout << "[EvaTerm] Reloading configuration...\n";
    if (!config_.config_file_path.empty()) {
        config_ = Config::load_from_file(config_.config_file_path);
    } else {
        config_ = Config::load_default();
    }

    tab_bar_->set_height(config_.tab_bar_height);
    font_atlas_.init(config_.font_family, config_.font_size);
    renderer_->update_config(config_);
    tab_manager_->update_theme(config_.theme);
    update_grid_dimensions();
}

void Window::update_window_title() {
    auto active_tab = tab_manager_->get_active_tab();
    if (active_tab) {
        std::string title = "EvaTerm - " + active_tab->get_title();
        SDL_SetWindowTitle(sdl_window_, title.c_str());
    } else {
        SDL_SetWindowTitle(sdl_window_, "EvaTerm");
    }
}

void Window::run() {
    is_running_ = true;
    SDL_Event event;

    while (is_running_) {
        // 1. Check inotify for live config file modifications (hot-reload)
        check_config_watcher();

        // 2. Process all pending window and input events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running_ = false;
                break;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED || event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    handle_resize(event.window.data1, event.window.data2);
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                    is_focused_ = true;
                } else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                    is_focused_ = false;
                }
            } else {
                input_handler_->handle_event(event, window_width_, window_height_);
            }
        }

        // 3. Poll PTY sessions for all tabs
        bool has_alive_tabs = tab_manager_->update_all();
        if (!has_alive_tabs) {
            // All tabs closed -> exit
            is_running_ = false;
            break;
        }

        // 4. Update tab bar layout and window title
        tab_bar_->update_layout(*tab_manager_, window_width_);
        update_window_title();

        // 5. Render frame
        renderer_->render(*tab_manager_, *tab_bar_, window_width_, window_height_, is_focused_);
        SDL_GL_SwapWindow(sdl_window_);

        // 6. Small sleep to avoid busy looping while keeping high responsiveness
        SDL_Delay(8); // ~120 Hz update rate
    }
}

} // namespace evaterm
