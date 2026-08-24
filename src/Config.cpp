#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace evaterm {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void Config::apply_theme_by_name(const std::string& name) {
    theme_name = name;
    std::string lower = name;
    for (char& c : lower) c = std::tolower(c);

    if (lower == "crimson_flame" || lower == "crimson" || lower == "crimson-flame") {
        theme = Theme::CrimsonFlame();
    } else if (lower == "catppuccin_mocha" || lower == "catppuccin" || lower == "mocha") {
        theme = Theme::CatppuccinMocha();
    } else if (lower == "tokyo_night" || lower == "tokyonight") {
        theme = Theme::TokyoNight();
    } else if (lower == "dracula") {
        theme = Theme::Dracula();
    } else if (lower == "nord") {
        theme = Theme::Nord();
    } else if (lower == "gruvbox_dark" || lower == "gruvbox") {
        theme = Theme::GruvboxDark();
    } else {
        if (fs::exists(name)) {
            theme = Theme::load_from_file(name);
        } else {
            theme = Theme::CrimsonFlame();
        }
    }
}

void Config::parse_line(const std::string& raw_line, const std::string& base_dir) {
    std::string line = trim(raw_line);
    if (line.empty() || line[0] == '#' || line[0] == ';') return;

    size_t split_pos = line.find('=');
    if (split_pos == std::string::npos) {
        split_pos = line.find_first_of(" \t");
    }
    if (split_pos == std::string::npos) return;

    std::string key = trim(line.substr(0, split_pos));
    std::string val = trim(line.substr(split_pos + 1));

    // Remove quotes
    if (val.length() >= 2 && (val.front() == '"' || val.front() == '\'') && val.front() == val.back()) {
        val = val.substr(1, val.length() - 2);
    }

    if (key == "include") {
        std::string include_path = val;
        if (!base_dir.empty() && !fs::path(include_path).is_absolute()) {
            include_path = base_dir + "/" + include_path;
        }
        std::ifstream inc_file(include_path);
        if (inc_file.is_open()) {
            std::string inc_line;
            while (std::getline(inc_file, inc_line)) {
                parse_line(inc_line, fs::path(include_path).parent_path().string());
            }
        }
        return;
    }

    // Font settings
    if (key == "font_family") {
        font_family = val;
    } else if (key == "font_size") {
        try { font_size = static_cast<int>(std::stof(val)); } catch (...) {}
    }
    // Shell & Behavior
    else if (key == "shell") {
        shell_path = val;
    } else if (key == "scrollback_limit" || key == "scrollback_lines") {
        try { scrollback_limit = std::stoul(val); } catch (...) {}
    } else if (key == "cursor_shape") {
        if (val == "beam" || val == "bar") cursor_shape = CursorShape::Beam;
        else if (val == "underline" || val == "underscore") cursor_shape = CursorShape::Underline;
        else cursor_shape = CursorShape::Block;
    } else if (key == "cursor_blink") {
        cursor_blink = (val == "true" || val == "1" || val == "yes" || val == "on");
    } else if (key == "cursor_blink_interval" || key == "cursor_blink_interval_ms") {
        try { cursor_blink_interval_ms = std::stoi(val); } catch (...) {}
    } else if (key == "padding_x" || key == "window_padding_width") {
        try { padding_x = std::stoi(val); } catch (...) {}
    } else if (key == "padding_y") {
        try { padding_y = std::stoi(val); } catch (...) {}
    } else if (key == "show_tab_bar_single_tab") {
        show_tab_bar_single_tab = (val == "true" || val == "1" || val == "yes" || val == "on");
    } else if (key == "tab_bar_height") {
        try { tab_bar_height = std::stoi(val); } catch (...) {}
    }
    // Theme preset
    else if (key == "theme") {
        apply_theme_by_name(val);
    }
    // Direct Color Overrides
    else if (key == "background") {
        theme.background = Color::from_hex(val);
    } else if (key == "foreground") {
        theme.foreground = Color::from_hex(val);
    } else if (key == "cursor") {
        theme.cursor = Color::from_hex(val);
    } else if (key == "cursor_text" || key == "cursor_text_color") {
        theme.cursor_text = Color::from_hex(val);
    } else if (key == "selection_bg" || key == "selection_background") {
        theme.selection_bg = Color::from_hex(val);
    } else if (key == "selection_fg" || key == "selection_foreground") {
        theme.selection_fg = Color::from_hex(val);
    } else if (key == "active_tab_bg" || key == "active_tab_background") {
        theme.active_tab_bg = Color::from_hex(val);
    } else if (key == "active_tab_fg" || key == "active_tab_foreground") {
        theme.active_tab_fg = Color::from_hex(val);
    } else if (key == "inactive_tab_bg" || key == "inactive_tab_background") {
        theme.inactive_tab_bg = Color::from_hex(val);
    } else if (key == "inactive_tab_fg" || key == "inactive_tab_foreground") {
        theme.inactive_tab_fg = Color::from_hex(val);
    } else if (key == "tab_bar_bg" || key == "tab_bar_background") {
        theme.tab_bar_bg = Color::from_hex(val);
    } else if (key == "background_opacity") {
        try { theme.background_opacity = std::stof(val); } catch (...) {}
    } else if (key.rfind("color", 0) == 0) {
        try {
            int color_idx = std::stoi(key.substr(5));
            if (color_idx >= 0 && color_idx < 16) {
                theme.ansi_colors[color_idx] = Color::from_hex(val);
            }
        } catch (...) {}
    }
}

Config Config::load_default() {
    Config cfg;
    cfg.apply_theme_by_name("crimson_flame");

    const char* home = std::getenv("HOME");
    if (home) {
        std::string config_dir = std::string(home) + "/.config/evaterm";
        std::string conf_file = config_dir + "/evaterm.conf";
        std::string ini_file = config_dir + "/config.ini";

        if (fs::exists(conf_file)) {
            return load_from_file(conf_file);
        } else if (fs::exists(ini_file)) {
            return load_from_file(ini_file);
        } else {
            try {
                fs::create_directories(config_dir);
                cfg.config_file_path = conf_file;
                cfg.save_to_file(conf_file);
            } catch (...) {}
        }
    }
    return cfg;
}

Config Config::load_from_file(const std::string& filepath) {
    Config cfg;
    cfg.config_file_path = filepath;
    cfg.apply_theme_by_name("crimson_flame"); // default base

    std::ifstream file(filepath);
    if (!file.is_open()) return cfg;

    std::string base_dir = fs::path(filepath).parent_path().string();
    std::string line;
    while (std::getline(file, line)) {
        cfg.parse_line(line, base_dir);
    }

    return cfg;
}

bool Config::save_to_file(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "# ====================================================================\n";
    out << "#  EvaTerm Configuration File (" << filepath << ")\n";
    out << "#  Live changes are hot-reloaded automatically on save!\n";
    out << "# ====================================================================\n\n";

    out << "# --- Theme Preset ---\n";
    out << "# Options: crimson_flame, catppuccin_mocha, tokyo_night, dracula, nord, gruvbox_dark\n";
    out << "theme " << theme_name << "\n\n";

    out << "# --- Font Configuration ---\n";
    out << "font_family " << font_family << "\n";
    out << "font_size   " << font_size << "\n\n";

    out << "# --- Window & Padding ---\n";
    out << "background_opacity " << theme.background_opacity << "\n";
    out << "padding_x          " << padding_x << "\n";
    out << "padding_y          " << padding_y << "\n\n";

    out << "# --- Tab Bar ---\n";
    out << "show_tab_bar_single_tab " << (show_tab_bar_single_tab ? "yes" : "no") << "\n";
    out << "tab_bar_height          " << tab_bar_height << "\n\n";

    out << "# --- Shell & Terminal Behavior ---\n";
    out << "shell            " << shell_path << "\n";
    out << "scrollback_lines " << scrollback_limit << "\n";
    out << "cursor_shape     " << (cursor_shape == CursorShape::Beam ? "beam" : (cursor_shape == CursorShape::Underline ? "underline" : "block")) << "\n";
    out << "cursor_blink     " << (cursor_blink ? "yes" : "no") << "\n";
    out << "cursor_blink_interval " << cursor_blink_interval_ms << "\n\n";

    out << "# --- Color Scheme Overrides (Optional) ---\n";
    out << "# You can customize any color directly:\n";
    out << "background           " << theme.background.to_hex() << "\n";
    out << "foreground           " << theme.foreground.to_hex() << "\n";
    out << "cursor               " << theme.cursor.to_hex() << "\n";
    out << "cursor_text_color    " << theme.cursor_text.to_hex() << "\n";
    out << "selection_background " << theme.selection_bg.to_hex() << "\n";
    out << "selection_foreground " << theme.selection_fg.to_hex() << "\n\n";

    out << "# Tab Bar Colors\n";
    out << "active_tab_background   " << theme.active_tab_bg.to_hex() << "\n";
    out << "active_tab_foreground   " << theme.active_tab_fg.to_hex() << "\n";
    out << "inactive_tab_background " << theme.inactive_tab_bg.to_hex() << "\n";
    out << "inactive_tab_foreground " << theme.inactive_tab_fg.to_hex() << "\n";
    out << "tab_bar_background      " << theme.tab_bar_bg.to_hex() << "\n\n";

    out << "# 16 Terminal ANSI Colors\n";
    for (size_t i = 0; i < 16; ++i) {
        out << "color" << i << " " << theme.ansi_colors[i].to_hex() << "\n";
    }

    return true;
}

} // namespace evaterm
