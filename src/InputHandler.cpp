#include "InputHandler.hpp"
#include <iostream>

namespace evaterm {

InputHandler::InputHandler(TabManager& tab_manager, FontAtlas& font_atlas, Config& config, TabBar& tab_bar)
    : tab_manager_(tab_manager),
      font_atlas_(font_atlas),
      config_(config),
      tab_bar_(tab_bar) {}

bool InputHandler::handle_event(const SDL_Event& event, int window_width, int window_height) {
    switch (event.type) {
        case SDL_KEYDOWN:
            return handle_keydown(event.key);
        case SDL_TEXTINPUT:
            return handle_text_input(event.text);
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            return handle_mouse_button(event.button, window_width, window_height);
        case SDL_MOUSEMOTION:
            return handle_mouse_motion(event.motion, window_width, window_height);
        case SDL_MOUSEWHEEL:
            return handle_mouse_wheel(event.wheel);
        default:
            return false;
    }
}

bool InputHandler::handle_keydown(const SDL_KeyboardEvent& key) {
    SDL_Keycode sym = key.keysym.sym;
    Uint16 mod = key.keysym.mod;
    bool ctrl = (mod & KMOD_CTRL) != 0;
    bool shift = (mod & KMOD_SHIFT) != 0;
    bool alt = (mod & KMOD_ALT) != 0;

    auto active_tab = tab_manager_.get_active_tab();

    // 1. Check Global Application Shortcuts
    if (ctrl && shift) {
        if (sym == SDLK_t) {
            tab_manager_.create_tab();
            return true;
        }
        if (sym == SDLK_w) {
            tab_manager_.close_active_tab();
            return true;
        }
        if (sym == SDLK_c) {
            copy_to_clipboard();
            return true;
        }
        if (sym == SDLK_v) {
            paste_from_clipboard();
            return true;
        }
        if (sym == SDLK_r) {
            if (on_config_reload_) on_config_reload_();
            return true;
        }
        if (sym == SDLK_TAB) {
            tab_manager_.prev_tab();
            return true;
        }
    }

    if (ctrl) {
        if (sym == SDLK_TAB && !shift) {
            tab_manager_.next_tab();
            return true;
        }
        if (sym == SDLK_PLUS || sym == SDLK_EQUALS || sym == SDLK_KP_PLUS) {
            if (on_font_resize_) on_font_resize_(font_atlas_.get_font_size() + 1);
            return true;
        }
        if (sym == SDLK_MINUS || sym == SDLK_KP_MINUS) {
            if (on_font_resize_) on_font_resize_(font_atlas_.get_font_size() - 1);
            return true;
        }
        if (sym == SDLK_0 || sym == SDLK_KP_0) {
            if (on_font_resize_) on_font_resize_(config_.font_size);
            return true;
        }
    }

    if (alt && !ctrl && !shift) {
        if (sym >= SDLK_1 && sym <= SDLK_9) {
            size_t target_tab = sym - SDLK_1;
            tab_manager_.switch_tab(target_tab);
            return true;
        }
    }

    if (shift && !ctrl && !alt) {
        if (sym == SDLK_PAGEUP) {
            if (active_tab) active_tab->get_terminal().get_buffer().scroll_viewport(10);
            return true;
        }
        if (sym == SDLK_PAGEDOWN) {
            if (active_tab) active_tab->get_terminal().get_buffer().scroll_viewport(-10);
            return true;
        }
    }

    if (!active_tab) return false;

    auto& terminal = active_tab->get_terminal();
    bool app_cursor = terminal.get_buffer().is_app_cursor_keys();

    // 2. Standard Terminal Navigation & Control Keys
    switch (sym) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            terminal.write_input("\r");
            return true;
        case SDLK_BACKSPACE:
            terminal.write_input("\x7f");
            return true;
        case SDLK_TAB:
            terminal.write_input("\t");
            return true;
        case SDLK_ESCAPE:
            terminal.write_input("\x1b");
            return true;
        case SDLK_UP:
            terminal.write_input(app_cursor ? "\033OA" : "\033[A");
            return true;
        case SDLK_DOWN:
            terminal.write_input(app_cursor ? "\033OB" : "\033[B");
            return true;
        case SDLK_RIGHT:
            terminal.write_input(app_cursor ? "\033OC" : "\033[C");
            return true;
        case SDLK_LEFT:
            terminal.write_input(app_cursor ? "\033OD" : "\033[D");
            return true;
        case SDLK_HOME:
            terminal.write_input("\033[H");
            return true;
        case SDLK_END:
            terminal.write_input("\033[F");
            return true;
        case SDLK_INSERT:
            terminal.write_input("\033[2~");
            return true;
        case SDLK_DELETE:
            terminal.write_input("\033[3~");
            return true;
        case SDLK_PAGEUP:
            terminal.write_input("\033[5~");
            return true;
        case SDLK_PAGEDOWN:
            terminal.write_input("\033[6~");
            return true;
        case SDLK_F1:  terminal.write_input("\033OP"); return true;
        case SDLK_F2:  terminal.write_input("\033OQ"); return true;
        case SDLK_F3:  terminal.write_input("\033OR"); return true;
        case SDLK_F4:  terminal.write_input("\033OS"); return true;
        case SDLK_F5:  terminal.write_input("\033[15~"); return true;
        case SDLK_F6:  terminal.write_input("\033[17~"); return true;
        case SDLK_F7:  terminal.write_input("\033[18~"); return true;
        case SDLK_F8:  terminal.write_input("\033[19~"); return true;
        case SDLK_F9:  terminal.write_input("\033[20~"); return true;
        case SDLK_F10: terminal.write_input("\033[21~"); return true;
        case SDLK_F11: terminal.write_input("\033[23~"); return true;
        case SDLK_F12: terminal.write_input("\033[24~"); return true;
        default:
            break;
    }

    // 3. Ctrl + letter keys (e.g. Ctrl+C -> \x03, Ctrl+D -> \x04, etc.)
    if (ctrl && !shift && !alt) {
        if (sym >= SDLK_a && sym <= SDLK_z) {
            char ctrl_char = static_cast<char>(sym - SDLK_a + 1);
            terminal.write_input(&ctrl_char, 1);
            return true;
        }
        if (sym == SDLK_LEFTBRACKET) { // Ctrl+[ (ESC)
            terminal.write_input("\x1b");
            return true;
        }
        if (sym == SDLK_BACKSLASH) { // Ctrl+\ (SIGQUIT)
            terminal.write_input("\x1c");
            return true;
        }
        if (sym == SDLK_RIGHTBRACKET) { // Ctrl+]
            terminal.write_input("\x1d");
            return true;
        }
    }

    return false;
}

bool InputHandler::handle_text_input(const SDL_TextInputEvent& text) {
    auto active_tab = tab_manager_.get_active_tab();
    if (!active_tab) return false;

    // Normal text character entry
    active_tab->get_terminal().write_input(text.text);
    return true;
}

bool InputHandler::handle_mouse_button(const SDL_MouseButtonEvent& button, int win_w, int win_h) {
    float mx = static_cast<float>(button.x);
    float my = static_cast<float>(button.y);

    bool show_tabs = config_.show_tab_bar_single_tab || tab_manager_.get_tab_count() > 1;
    int tab_bar_h = show_tabs ? tab_bar_.get_height() : 0;

    if (button.type == SDL_MOUSEBUTTONDOWN) {
        // Tab Bar clicks
        if (show_tabs && my <= tab_bar_h) {
            int close_tab = tab_bar_.get_close_button_at_point(mx, my);
            if (close_tab >= 0 && button.button == SDL_BUTTON_LEFT) {
                tab_manager_.close_tab(static_cast<size_t>(close_tab));
                return true;
            }

            if (tab_bar_.is_add_button_at_point(mx, my) && button.button == SDL_BUTTON_LEFT) {
                tab_manager_.create_tab();
                return true;
            }

            int clicked_tab = tab_bar_.get_tab_at_point(mx, my);
            if (clicked_tab >= 0) {
                if (button.button == SDL_BUTTON_LEFT) {
                    tab_manager_.switch_tab(static_cast<size_t>(clicked_tab));
                } else if (button.button == SDL_BUTTON_MIDDLE) {
                    tab_manager_.close_tab(static_cast<size_t>(clicked_tab));
                }
                return true;
            }
            return true;
        }

        // Terminal Grid clicks
        auto active_tab = tab_manager_.get_active_tab();
        if (active_tab) {
            auto& buffer = active_tab->get_terminal().get_buffer();

            if (button.button == SDL_BUTTON_LEFT) {
                int cell_w = font_atlas_.get_cell_width();
                int cell_h = font_atlas_.get_cell_height();
                int col = static_cast<int>((mx - config_.padding_x) / cell_w);
                int row = static_cast<int>((my - tab_bar_h - config_.padding_y) / cell_h);

                if (button.clicks == 1) {
                    buffer.start_selection(row, col);
                    selecting_ = true;
                } else if (button.clicks == 2) {
                    buffer.select_word(row, col);
                    selecting_ = false;
                } else if (button.clicks >= 3) {
                    buffer.select_line(row);
                    selecting_ = false;
                }
                return true;
            } else if (button.button == SDL_BUTTON_MIDDLE) {
                // Middle click paste
                paste_from_clipboard();
                return true;
            }
        }
    } else if (button.type == SDL_MOUSEBUTTONUP) {
        if (button.button == SDL_BUTTON_LEFT && selecting_) {
            selecting_ = false;
            // Optionally auto-copy on selection
            return true;
        }
    }

    return false;
}

bool InputHandler::handle_mouse_motion(const SDL_MouseMotionEvent& motion, int win_w, int win_h) {
    float mx = static_cast<float>(motion.x);
    float my = static_cast<float>(motion.y);

    bool show_tabs = config_.show_tab_bar_single_tab || tab_manager_.get_tab_count() > 1;
    int tab_bar_h = show_tabs ? tab_bar_.get_height() : 0;

    // Update Tab bar hover states
    if (show_tabs && my <= tab_bar_h) {
        int tab_idx = tab_bar_.get_tab_at_point(mx, my);
        int close_idx = tab_bar_.get_close_button_at_point(mx, my);
        bool add_btn = tab_bar_.is_add_button_at_point(mx, my);
        tab_bar_.set_hover_state(tab_idx, close_idx, add_btn);
    } else {
        tab_bar_.set_hover_state(-1, -1, false);
    }

    // Update Drag selection in terminal
    if (selecting_) {
        auto active_tab = tab_manager_.get_active_tab();
        if (active_tab) {
            auto& buffer = active_tab->get_terminal().get_buffer();
            int cell_w = font_atlas_.get_cell_width();
            int cell_h = font_atlas_.get_cell_height();
            int col = static_cast<int>((mx - config_.padding_x) / cell_w);
            int row = static_cast<int>((my - tab_bar_h - config_.padding_y) / cell_h);
            buffer.update_selection(row, col);
            return true;
        }
    }

    return false;
}

bool InputHandler::handle_mouse_wheel(const SDL_MouseWheelEvent& wheel) {
    auto active_tab = tab_manager_.get_active_tab();
    if (!active_tab) return false;

    int delta = (wheel.y > 0) ? 3 : -3;
    active_tab->get_terminal().get_buffer().scroll_viewport(delta);
    return true;
}

void InputHandler::copy_to_clipboard() {
    auto active_tab = tab_manager_.get_active_tab();
    if (!active_tab) return;

    std::string text = active_tab->get_terminal().get_buffer().get_selected_text();
    if (!text.empty()) {
        SDL_SetClipboardText(text.c_str());
    }
}

void InputHandler::paste_from_clipboard() {
    auto active_tab = tab_manager_.get_active_tab();
    if (!active_tab) return;

    if (SDL_HasClipboardText()) {
        char* clip_text = SDL_GetClipboardText();
        if (clip_text) {
            auto& buffer = active_tab->get_terminal().get_buffer();
            if (buffer.is_bracketed_paste_mode()) {
                active_tab->get_terminal().write_input("\033[200~");
                active_tab->get_terminal().write_input(clip_text);
                active_tab->get_terminal().write_input("\033[201~");
            } else {
                active_tab->get_terminal().write_input(clip_text);
            }
            SDL_free(clip_text);
        }
    }
}

} // namespace evaterm
