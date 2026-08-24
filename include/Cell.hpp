#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace evaterm {

struct Color {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
        : r(red), g(green), b(blue), a(alpha) {}

    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    bool operator!=(const Color& other) const {
        return !(*this == other);
    }

    static Color from_hex(const std::string& hex_str) {
        std::string hex = hex_str;
        if (!hex.empty() && hex[0] == '#') {
            hex = hex.substr(1);
        }
        if (hex.length() == 6) {
            uint32_t val = 0;
            std::stringstream ss;
            ss << std::hex << hex;
            ss >> val;
            return Color(
                static_cast<uint8_t>((val >> 16) & 0xFF),
                static_cast<uint8_t>((val >> 8) & 0xFF),
                static_cast<uint8_t>(val & 0xFF)
            );
        } else if (hex.length() == 8) {
            uint32_t val = 0;
            std::stringstream ss;
            ss << std::hex << hex;
            ss >> val;
            return Color(
                static_cast<uint8_t>((val >> 24) & 0xFF),
                static_cast<uint8_t>((val >> 16) & 0xFF),
                static_cast<uint8_t>((val >> 8) & 0xFF),
                static_cast<uint8_t>(val & 0xFF)
            );
        }
        return Color(255, 255, 255, 255);
    }

    std::string to_hex() const {
        std::stringstream ss;
        ss << "#" << std::hex << std::setfill('0')
           << std::setw(2) << static_cast<int>(r)
           << std::setw(2) << static_cast<int>(g)
           << std::setw(2) << static_cast<int>(b);
        return ss.str();
    }
};

enum CellAttributeFlags : uint16_t {
    ATTR_NONE          = 0,
    ATTR_BOLD          = 1 << 0,
    ATTR_DIM           = 1 << 1,
    ATTR_ITALIC        = 1 << 2,
    ATTR_UNDERLINE     = 1 << 3,
    ATTR_BLINK         = 1 << 4,
    ATTR_INVERSE       = 1 << 5,
    ATTR_HIDDEN        = 1 << 6,
    ATTR_STRIKETHROUGH = 1 << 7,
    ATTR_URL           = 1 << 8
};

enum class CursorShape {
    Block,
    Beam,
    Underline
};

struct Cell {
    char32_t codepoint = U' ';
    Color fg;
    Color bg;
    uint16_t attributes = ATTR_NONE;
    bool is_default_fg = true;
    bool is_default_bg = true;

    bool is_empty() const {
        return (codepoint == U' ' || codepoint == 0) && is_default_bg && (attributes == ATTR_NONE);
    }

    void reset(const Color& def_fg, const Color& def_bg) {
        codepoint = U' ';
        fg = def_fg;
        bg = def_bg;
        attributes = ATTR_NONE;
        is_default_fg = true;
        is_default_bg = true;
    }
};

} // namespace evaterm
