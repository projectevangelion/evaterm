#pragma once

#include "Cell.hpp"
#include <string>
#include <array>
#include <unordered_map>

namespace evaterm {

struct Theme {
    std::string name = "Crimson Flame";
    Color foreground = Color::from_hex("#ebdada");
    Color background = Color::from_hex("#25090a");
    Color cursor = Color::from_hex("#e32a10");
    Color cursor_text = Color::from_hex("#25090a");
    Color selection_bg = Color::from_hex("#cf2824");
    Color selection_fg = Color::from_hex("#ffffff");
    
    // Tab bar theming
    Color active_tab_bg = Color::from_hex("#cf2824");
    Color active_tab_fg = Color::from_hex("#ffffff");
    Color inactive_tab_bg = Color::from_hex("#360e10");
    Color inactive_tab_fg = Color::from_hex("#b58487");
    Color tab_bar_bg = Color::from_hex("#1a0506");
    Color tab_bar_border = Color::from_hex("#4a1215");

    // 16 Standard & Bright ANSI colors
    std::array<Color, 16> ansi_colors = {
        Color::from_hex("#330d0f"), // 0: black
        Color::from_hex("#cf2824"), // 1: red
        Color::from_hex("#e0682b"), // 2: green (orange-red accent)
        Color::from_hex("#e6ab3c"), // 3: yellow
        Color::from_hex("#ff7373"), // 4: blue (light red accent)
        Color::from_hex("#bd5486"), // 5: magenta
        Color::from_hex("#ff9f68"), // 6: cyan (light orange accent)
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

    float background_opacity = 1.0f;

    Color get_256_color(uint8_t index) const;

    static Theme CrimsonFlame();
    static Theme CatppuccinMocha();
    static Theme TokyoNight();
    static Theme Dracula();
    static Theme Nord();
    static Theme GruvboxDark();

    static Theme load_from_file(const std::string& filepath);
    bool save_to_file(const std::string& filepath) const;
};

} // namespace evaterm
