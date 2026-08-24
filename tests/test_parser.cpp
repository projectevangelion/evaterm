#include "AnsiParser.hpp"
#include "ScreenBuffer.hpp"
#include "Theme.hpp"
#include <cassert>
#include <iostream>

void test_basic_text() {
    evaterm::Theme theme = evaterm::Theme::CrimsonFlame();
    evaterm::ScreenBuffer buffer(24, 80, theme.foreground, theme.background);
    evaterm::AnsiParser parser(buffer, theme);

    std::string text = "Hello EvaTerm!";
    parser.parse(text.data(), text.size());

    assert(buffer.get_cursor_row() == 0);
    assert(buffer.get_cursor_col() == 14);

    for (size_t i = 0; i < text.size(); ++i) {
        assert(buffer.get_cell(0, static_cast<int>(i)).codepoint == static_cast<char32_t>(text[i]));
    }
    std::cout << "[PASS] test_basic_text\n";
}

void test_sgr_colors() {
    evaterm::Theme theme = evaterm::Theme::CrimsonFlame();
    evaterm::ScreenBuffer buffer(24, 80, theme.foreground, theme.background);
    evaterm::AnsiParser parser(buffer, theme);

    // Red text (color 1)
    std::string red_seq = "\033[31mRed";
    parser.parse(red_seq.data(), red_seq.size());

    const auto& cell = buffer.get_cell(0, 0);
    assert(cell.fg == theme.ansi_colors[1]);
    assert(cell.codepoint == U'R');

    // TrueColor 24-bit RGB \033[38;2;200;100;50m
    std::string tc_seq = "\033[38;2;200;100;50mTC";
    parser.parse(tc_seq.data(), tc_seq.size());

    const auto& cell_tc = buffer.get_cell(0, 3);
    assert(cell_tc.fg.r == 200);
    assert(cell_tc.fg.g == 100);
    assert(cell_tc.fg.b == 50);

    std::cout << "[PASS] test_sgr_colors\n";
}

void test_cursor_movement_and_clear() {
    evaterm::Theme theme = evaterm::Theme::CrimsonFlame();
    evaterm::ScreenBuffer buffer(24, 80, theme.foreground, theme.background);
    evaterm::AnsiParser parser(buffer, theme);

    // Position cursor at row 5, col 10 (1-based: \033[6;11H)
    std::string cup_seq = "\033[6;11H";
    parser.parse(cup_seq.data(), cup_seq.size());
    assert(buffer.get_cursor_row() == 5);
    assert(buffer.get_cursor_col() == 10);

    // Cursor Up 2 lines (\033[2A)
    std::string cuu_seq = "\033[2A";
    parser.parse(cuu_seq.data(), cuu_seq.size());
    assert(buffer.get_cursor_row() == 3);

    // Clear Line (\033[2K)
    std::string el_seq = "\033[2K";
    parser.parse(el_seq.data(), el_seq.size());
    for (int c = 0; c < 80; ++c) {
        assert(buffer.get_cell(3, c).codepoint == U' ');
    }

    std::cout << "[PASS] test_cursor_movement_and_clear\n";
}

void test_osc_title() {
    evaterm::Theme theme = evaterm::Theme::CrimsonFlame();
    evaterm::ScreenBuffer buffer(24, 80, theme.foreground, theme.background);
    evaterm::AnsiParser parser(buffer, theme);

    std::string captured_title;
    parser.set_title_callback([&](const std::string& title) {
        captured_title = title;
    });

    std::string osc_seq = "\033]0;My Custom Project\007";
    parser.parse(osc_seq.data(), osc_seq.size());
    assert(captured_title == "My Custom Project");

    std::cout << "[PASS] test_osc_title\n";
}

void test_alt_screen_buffer() {
    evaterm::Theme theme = evaterm::Theme::CrimsonFlame();
    evaterm::ScreenBuffer buffer(24, 80, theme.foreground, theme.background);
    evaterm::AnsiParser parser(buffer, theme);

    // Enter alt buffer (\033[?1049h)
    std::string enter_alt = "\033[?1049h";
    parser.parse(enter_alt.data(), enter_alt.size());
    assert(buffer.is_alt_buffer_active());

    // Exit alt buffer (\033[?1049l)
    std::string exit_alt = "\033[?1049l";
    parser.parse(exit_alt.data(), exit_alt.size());
    assert(!buffer.is_alt_buffer_active());

    std::cout << "[PASS] test_alt_screen_buffer\n";
}

void test_charset_designation_escape() {
    evaterm::Theme theme = evaterm::Theme::CrimsonFlame();
    evaterm::ScreenBuffer buffer(24, 80, theme.foreground, theme.background);
    evaterm::AnsiParser parser(buffer, theme);

    // \033(B (US-ASCII designation) and \033)0 (DEC Line Drawing designation)
    std::string charset_seq = "\033(B\033)0";
    parser.parse(charset_seq.data(), charset_seq.size());

    // Must NOT print 'B' or '0' to the buffer!
    assert(buffer.get_cursor_row() == 0);
    assert(buffer.get_cursor_col() == 0);
    assert(buffer.get_cell(0, 0).codepoint == U' ');

    std::cout << "[PASS] test_charset_designation_escape\n";
}

void test_kitty_graphics_protocol() {
    evaterm::Theme theme = evaterm::Theme::CrimsonFlame();
    evaterm::ScreenBuffer buffer(24, 80, theme.foreground, theme.background);
    evaterm::AnsiParser parser(buffer, theme);

    // 1x1 Red pixel RGBA: /wAA/w==
    std::string kitty_seq = "\033_Ga=T,f=32,s=1,v=1,c=2,r=1,q=2;/wAA/w==\033\\";
    parser.parse(kitty_seq.data(), kitty_seq.size());

    const auto& images = buffer.get_image_placements();
    assert(images.size() == 1);
    assert(images[0].cols == 2);
    assert(images[0].rows == 1);
    assert(images[0].image != nullptr);
    assert(images[0].image->pixel_width == 1);
    assert(images[0].image->pixel_height == 1);
    assert(images[0].image->rgba_data.size() == 4);
    assert(images[0].image->rgba_data[0] == 0xFF); // Red

    std::cout << "[PASS] test_kitty_graphics_protocol\n";
}

int main() {
    std::cout << "Running AnsiParser Unit Tests...\n";
    test_basic_text();
    test_sgr_colors();
    test_cursor_movement_and_clear();
    test_osc_title();
    test_alt_screen_buffer();
    test_charset_designation_escape();
    test_kitty_graphics_protocol();
    std::cout << "All AnsiParser Tests Passed!\n";
    return 0;
}
