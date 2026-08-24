#include "ScreenBuffer.hpp"
#include "Theme.hpp"
#include <cassert>
#include <iostream>

void test_buffer_wrapping_and_scrollback() {
    evaterm::Color fg(200, 200, 200);
    evaterm::Color bg(20, 20, 20);
    evaterm::ScreenBuffer buffer(5, 10, fg, bg, 100);

    // Write 10 characters (fill line 0)
    for (int i = 0; i < 10; ++i) {
        buffer.write_char(U'0' + i, fg, bg, 0, true, true);
    }
    assert(buffer.get_cursor_row() == 0);
    assert(buffer.get_cursor_col() == 9);

    // 11th char wraps to next row
    buffer.write_char(U'X', fg, bg, 0, true, true);
    assert(buffer.get_cursor_row() == 1);
    assert(buffer.get_cursor_col() == 1);
    assert(buffer.get_cell(1, 0).codepoint == U'X');

    // Fill all 5 rows and trigger scroll up
    for (int r = 0; r < 10; ++r) {
        buffer.newline();
    }
    assert(buffer.get_scrollback_size() > 0);
    std::cout << "[PASS] test_buffer_wrapping_and_scrollback\n";
}

void test_buffer_selection() {
    evaterm::Color fg(200, 200, 200);
    evaterm::Color bg(20, 20, 20);
    evaterm::ScreenBuffer buffer(5, 20, fg, bg, 100);

    std::string word = "EVA_TERM";
    for (char c : word) {
        buffer.write_char(static_cast<char32_t>(c), fg, bg, 0, true, true);
    }

    // Only text length is 8 (cols 0..7)
    buffer.start_selection(0, 0);
    buffer.update_selection(0, 7);
    assert(buffer.has_selection());
    assert(buffer.is_cell_selected(0, 4));
    // Blank spaces past col 7 must NOT be selected
    assert(!buffer.is_cell_selected(0, 8));
    assert(!buffer.is_cell_selected(0, 15));

    std::string selected = buffer.get_selected_text();
    assert(selected == "EVA_TERM");

    // Clicking in blank space clears selection
    buffer.start_selection(0, 12);
    assert(!buffer.has_selection());

    // Test Word selection
    buffer.select_word(0, 3);
    assert(buffer.has_selection());
    assert(buffer.get_selected_text() == "EVA_TERM");

    // Test Line selection
    buffer.select_line(0);
    assert(buffer.has_selection());
    assert(buffer.get_selected_text() == "EVA_TERM");

    buffer.clear_selection();
    assert(!buffer.has_selection());

    std::cout << "[PASS] test_buffer_selection\n";
}

int main() {
    std::cout << "Running ScreenBuffer Unit Tests...\n";
    test_buffer_wrapping_and_scrollback();
    test_buffer_selection();
    std::cout << "All ScreenBuffer Tests Passed!\n";
    return 0;
}
