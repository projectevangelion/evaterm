#pragma once

#include "Theme.hpp"
#include <string>
#include <memory>

namespace evaterm {

struct Config {
    std::string font_family = "monospace";
    int font_size = 11;
    std::string shell_path = "";
    size_t scrollback_limit = 10000;
    CursorShape cursor_shape = CursorShape::Block;
    bool cursor_blink = true;
    int cursor_blink_interval_ms = 500;
    
    std::string theme_name = "crimson_flame";
    Theme theme;

    int padding_x = 8;
    int padding_y = 6;
    int tab_bar_height = 30;
    bool show_tab_bar_single_tab = true;

    std::string config_file_path;

    static Config load_default();
    static Config load_from_file(const std::string& filepath);
    bool save_to_file(const std::string& filepath) const;
    void apply_theme_by_name(const std::string& name);
    void parse_line(const std::string& line, const std::string& base_dir = "");
};

} // namespace evaterm
