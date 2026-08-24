#include "Theme.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

namespace evaterm {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

Color Theme::get_256_color(uint8_t index) const {
    if (index < 16) {
        return ansi_colors[index];
    }
    if (index >= 16 && index <= 231) {
        // 6x6x6 color cube
        uint8_t idx = index - 16;
        uint8_t r_idx = idx / 36;
        uint8_t g_idx = (idx % 36) / 6;
        uint8_t b_idx = idx % 6;

        auto step = [](uint8_t v) -> uint8_t {
            return v == 0 ? 0 : (v * 40 + 55);
        };

        return Color(step(r_idx), step(g_idx), step(b_idx));
    }
    // 232..255: grayscale ramp (24 steps)
    uint8_t gray = 8 + (index - 232) * 10;
    return Color(gray, gray, gray);
}

Theme Theme::CrimsonFlame() {
    Theme t;
    t.name = "Crimson Flame";
    t.background = Color::from_hex("#25090a");
    t.foreground = Color::from_hex("#ebdada");
    t.selection_bg = Color::from_hex("#cf2824");
    t.selection_fg = Color::from_hex("#ffffff");
    t.cursor = Color::from_hex("#e32a10");
    t.cursor_text = Color::from_hex("#25090a");

    t.active_tab_bg = Color::from_hex("#cf2824");
    t.active_tab_fg = Color::from_hex("#ffffff");
    t.inactive_tab_bg = Color::from_hex("#360e10");
    t.inactive_tab_fg = Color::from_hex("#b58487");
    t.tab_bar_bg = Color::from_hex("#1a0506");
    t.tab_bar_border = Color::from_hex("#4a1215");

    t.ansi_colors = {
        Color::from_hex("#330d0f"), // 0: black
        Color::from_hex("#cf2824"), // 1: red
        Color::from_hex("#e0682b"), // 2: green
        Color::from_hex("#e6ab3c"), // 3: yellow
        Color::from_hex("#ff7373"), // 4: blue
        Color::from_hex("#bd5486"), // 5: magenta
        Color::from_hex("#ff9f68"), // 6: cyan
        Color::from_hex("#cfbebe"), // 7: white
        Color::from_hex("#6e282c"), // 8: bright black
        Color::from_hex("#e32a10"), // 9: bright red
        Color::from_hex("#ff843d"), // 10: bright green
        Color::from_hex("#ffc85a"), // 11: bright yellow
        Color::from_hex("#ffa19c"), // 12: bright blue
        Color::from_hex("#de6ea4"), // 13: bright magenta
        Color::from_hex("#ffbe94"), // 14: bright cyan
        Color::from_hex("#faecec")  // 15: bright white
    };
    return t;
}

Theme Theme::CatppuccinMocha() {
    Theme t;
    t.name = "Catppuccin Mocha";
    t.background = Color::from_hex("#1e1e2e");
    t.foreground = Color::from_hex("#cdd6f4");
    t.selection_bg = Color::from_hex("#585b70");
    t.selection_fg = Color::from_hex("#cdd6f4");
    t.cursor = Color::from_hex("#f5e0dc");
    t.cursor_text = Color::from_hex("#11111b");

    t.active_tab_bg = Color::from_hex("#cba6f7");
    t.active_tab_fg = Color::from_hex("#11111b");
    t.inactive_tab_bg = Color::from_hex("#181825");
    t.inactive_tab_fg = Color::from_hex("#a6adc8");
    t.tab_bar_bg = Color::from_hex("#11111b");
    t.tab_bar_border = Color::from_hex("#313244");

    t.ansi_colors = {
        Color::from_hex("#45475a"), // 0
        Color::from_hex("#f38ba8"), // 1
        Color::from_hex("#a6e3a1"), // 2
        Color::from_hex("#f9e2af"), // 3
        Color::from_hex("#89b4fa"), // 4
        Color::from_hex("#f5c2e7"), // 5
        Color::from_hex("#94e2d5"), // 6
        Color::from_hex("#bac2de"), // 7
        Color::from_hex("#585b70"), // 8
        Color::from_hex("#f38ba8"), // 9
        Color::from_hex("#a6e3a1"), // 10
        Color::from_hex("#f9e2af"), // 11
        Color::from_hex("#89b4fa"), // 12
        Color::from_hex("#f5c2e7"), // 13
        Color::from_hex("#94e2d5"), // 14
        Color::from_hex("#a6adc8")  // 15
    };
    return t;
}

Theme Theme::TokyoNight() {
    Theme t;
    t.name = "Tokyo Night";
    t.background = Color::from_hex("#1a1b26");
    t.foreground = Color::from_hex("#c0caf5");
    t.selection_bg = Color::from_hex("#33467c");
    t.selection_fg = Color::from_hex("#c0caf5");
    t.cursor = Color::from_hex("#c0caf5");
    t.cursor_text = Color::from_hex("#1a1b26");

    t.active_tab_bg = Color::from_hex("#7aa2f7");
    t.active_tab_fg = Color::from_hex("#15161e");
    t.inactive_tab_bg = Color::from_hex("#16161e");
    t.inactive_tab_fg = Color::from_hex("#787c99");
    t.tab_bar_bg = Color::from_hex("#15161e");
    t.tab_bar_border = Color::from_hex("#24283b");

    t.ansi_colors = {
        Color::from_hex("#15161e"),
        Color::from_hex("#f7768e"),
        Color::from_hex("#9ece6a"),
        Color::from_hex("#e0af68"),
        Color::from_hex("#7aa2f7"),
        Color::from_hex("#bb9af7"),
        Color::from_hex("#7dcfff"),
        Color::from_hex("#a9b1d6"),
        Color::from_hex("#414868"),
        Color::from_hex("#f7768e"),
        Color::from_hex("#9ece6a"),
        Color::from_hex("#e0af68"),
        Color::from_hex("#7aa2f7"),
        Color::from_hex("#bb9af7"),
        Color::from_hex("#7dcfff"),
        Color::from_hex("#c0caf5")
    };
    return t;
}

Theme Theme::Dracula() {
    Theme t;
    t.name = "Dracula";
    t.background = Color::from_hex("#282a36");
    t.foreground = Color::from_hex("#f8f8f2");
    t.selection_bg = Color::from_hex("#44475a");
    t.selection_fg = Color::from_hex("#f8f8f2");
    t.cursor = Color::from_hex("#f8f8f2");
    t.cursor_text = Color::from_hex("#282a36");

    t.active_tab_bg = Color::from_hex("#bd93f9");
    t.active_tab_fg = Color::from_hex("#282a36");
    t.inactive_tab_bg = Color::from_hex("#21222c");
    t.inactive_tab_fg = Color::from_hex("#6272a4");
    t.tab_bar_bg = Color::from_hex("#191a21");
    t.tab_bar_border = Color::from_hex("#44475a");

    t.ansi_colors = {
        Color::from_hex("#21222c"),
        Color::from_hex("#ff5555"),
        Color::from_hex("#50fa7b"),
        Color::from_hex("#f1fa8c"),
        Color::from_hex("#bd93f9"),
        Color::from_hex("#ff79c6"),
        Color::from_hex("#8be9fd"),
        Color::from_hex("#f8f8f2"),
        Color::from_hex("#6272a4"),
        Color::from_hex("#ff6e6e"),
        Color::from_hex("#69ff94"),
        Color::from_hex("#ffffa5"),
        Color::from_hex("#d6acff"),
        Color::from_hex("#ff92df"),
        Color::from_hex("#a4ffff"),
        Color::from_hex("#ffffff")
    };
    return t;
}

Theme Theme::Nord() {
    Theme t;
    t.name = "Nord";
    t.background = Color::from_hex("#2e3440");
    t.foreground = Color::from_hex("#d8dee9");
    t.selection_bg = Color::from_hex("#434c5e");
    t.selection_fg = Color::from_hex("#eceff4");
    t.cursor = Color::from_hex("#d8dee9");
    t.cursor_text = Color::from_hex("#2e3440");

    t.active_tab_bg = Color::from_hex("#88c0d0");
    t.active_tab_fg = Color::from_hex("#2e3440");
    t.inactive_tab_bg = Color::from_hex("#3b4252");
    t.inactive_tab_fg = Color::from_hex("#d8dee9");
    t.tab_bar_bg = Color::from_hex("#2e3440");
    t.tab_bar_border = Color::from_hex("#4c566a");

    t.ansi_colors = {
        Color::from_hex("#3b4252"),
        Color::from_hex("#bf616a"),
        Color::from_hex("#a3be8c"),
        Color::from_hex("#ebcb8b"),
        Color::from_hex("#81a1c1"),
        Color::from_hex("#b48ead"),
        Color::from_hex("#88c0d0"),
        Color::from_hex("#e5e9f0"),
        Color::from_hex("#4c566a"),
        Color::from_hex("#bf616a"),
        Color::from_hex("#a3be8c"),
        Color::from_hex("#ebcb8b"),
        Color::from_hex("#81a1c1"),
        Color::from_hex("#b48ead"),
        Color::from_hex("#8fbcbb"),
        Color::from_hex("#eceff4")
    };
    return t;
}

Theme Theme::GruvboxDark() {
    Theme t;
    t.name = "Gruvbox Dark";
    t.background = Color::from_hex("#282828");
    t.foreground = Color::from_hex("#ebdbb2");
    t.selection_bg = Color::from_hex("#504945");
    t.selection_fg = Color::from_hex("#ebdbb2");
    t.cursor = Color::from_hex("#ebdbb2");
    t.cursor_text = Color::from_hex("#282828");

    t.active_tab_bg = Color::from_hex("#d79921");
    t.active_tab_fg = Color::from_hex("#282828");
    t.inactive_tab_bg = Color::from_hex("#3c3836");
    t.inactive_tab_fg = Color::from_hex("#a89984");
    t.tab_bar_bg = Color::from_hex("#1d2021");
    t.tab_bar_border = Color::from_hex("#504945");

    t.ansi_colors = {
        Color::from_hex("#282828"),
        Color::from_hex("#cc241d"),
        Color::from_hex("#98971a"),
        Color::from_hex("#d79921"),
        Color::from_hex("#458588"),
        Color::from_hex("#b16286"),
        Color::from_hex("#689d6a"),
        Color::from_hex("#a89984"),
        Color::from_hex("#928374"),
        Color::from_hex("#fb4934"),
        Color::from_hex("#b8bb26"),
        Color::from_hex("#fabd2f"),
        Color::from_hex("#83a598"),
        Color::from_hex("#d3869b"),
        Color::from_hex("#8ec07c"),
        Color::from_hex("#ebdbb2")
    };
    return t;
}

Theme Theme::load_from_file(const std::string& filepath) {
    Theme t = Theme::CrimsonFlame();
    std::ifstream file(filepath);
    if (!file.is_open()) return t;

    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            // Also support kitty conf space syntax: key value
            eq_pos = line.find(' ');
            if (eq_pos == std::string::npos) continue;
        }

        std::string key = trim(line.substr(0, eq_pos));
        std::string val = trim(line.substr(eq_pos + 1));

        if (key == "name") t.name = val;
        else if (key == "background") t.background = Color::from_hex(val);
        else if (key == "foreground") t.foreground = Color::from_hex(val);
        else if (key == "cursor") t.cursor = Color::from_hex(val);
        else if (key == "cursor_text" || key == "cursor_text_color") t.cursor_text = Color::from_hex(val);
        else if (key == "selection_bg" || key == "selection_background") t.selection_bg = Color::from_hex(val);
        else if (key == "selection_fg" || key == "selection_foreground") t.selection_fg = Color::from_hex(val);
        else if (key == "active_tab_bg" || key == "active_tab_background") t.active_tab_bg = Color::from_hex(val);
        else if (key == "active_tab_fg" || key == "active_tab_foreground") t.active_tab_fg = Color::from_hex(val);
        else if (key == "inactive_tab_bg" || key == "inactive_tab_background") t.inactive_tab_bg = Color::from_hex(val);
        else if (key == "inactive_tab_fg" || key == "inactive_tab_foreground") t.inactive_tab_fg = Color::from_hex(val);
        else if (key == "tab_bar_bg" || key == "tab_bar_background") t.tab_bar_bg = Color::from_hex(val);
        else if (key == "background_opacity") {
            try { t.background_opacity = std::stof(val); } catch (...) {}
        }
        else if (key.rfind("color", 0) == 0) {
            try {
                int color_idx = std::stoi(key.substr(5));
                if (color_idx >= 0 && color_idx < 16) {
                    t.ansi_colors[color_idx] = Color::from_hex(val);
                }
            } catch (...) {}
        }
    }

    return t;
}

bool Theme::save_to_file(const std::string& filepath) const {
    std::ofstream out(filepath);
    if (!out.is_open()) return false;

    out << "# EvaTerm Theme Configuration\n";
    out << "name = " << name << "\n\n";
    out << "background = " << background.to_hex() << "\n";
    out << "foreground = " << foreground.to_hex() << "\n";
    out << "cursor = " << cursor.to_hex() << "\n";
    out << "cursor_text = " << cursor_text.to_hex() << "\n";
    out << "selection_bg = " << selection_bg.to_hex() << "\n";
    out << "selection_fg = " << selection_fg.to_hex() << "\n\n";

    out << "# Tab Bar\n";
    out << "active_tab_bg = " << active_tab_bg.to_hex() << "\n";
    out << "active_tab_fg = " << active_tab_fg.to_hex() << "\n";
    out << "inactive_tab_bg = " << inactive_tab_bg.to_hex() << "\n";
    out << "inactive_tab_fg = " << inactive_tab_fg.to_hex() << "\n";
    out << "tab_bar_bg = " << tab_bar_bg.to_hex() << "\n\n";

    out << "# 16 ANSI Colors\n";
    for (size_t i = 0; i < 16; ++i) {
        out << "color" << i << " = " << ansi_colors[i].to_hex() << "\n";
    }

    return true;
}

} // namespace evaterm
