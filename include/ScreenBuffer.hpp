#pragma once

#include "Cell.hpp"
#include "Image.hpp"
#include <vector>
#include <deque>
#include <string>
#include <memory>
#include <algorithm>
#include <unordered_map>

namespace evaterm {

struct CursorState {
    int row = 0;
    int col = 0;
    Color fg;
    Color bg;
    uint16_t attributes = ATTR_NONE;
    bool is_default_fg = true;
    bool is_default_bg = true;
    bool visible = true;
};

struct Selection {
    bool active = false;
    int start_row = 0;
    int start_col = 0;
    int end_row = 0;
    int end_col = 0;

    void normalize(int& r0, int& c0, int& r1, int& c1) const {
        if (start_row < end_row || (start_row == end_row && start_col <= end_col)) {
            r0 = start_row; c0 = start_col;
            r1 = end_row;   c1 = end_col;
        } else {
            r0 = end_row;   c0 = end_col;
            r1 = start_row; c1 = start_col;
        }
    }

    bool contains(int row, int col) const {
        if (!active) return false;
        int r0, c0, r1, c1;
        normalize(r0, c0, r1, c1);
        if (row < r0 || row > r1) return false;
        if (row == r0 && row == r1) return col >= c0 && col <= c1;
        if (row == r0) return col >= c0;
        if (row == r1) return col <= c1;
        return true;
    }
};

class ScreenBuffer {
public:
    ScreenBuffer(int rows, int cols, const Color& def_fg, const Color& def_bg, size_t scrollback_max = 10000);

    void resize(int rows, int cols);
    void reset();

    // Text & Character Writing
    void write_char(char32_t codepoint, const Color& fg, const Color& bg, uint16_t attrs, bool def_fg, bool def_bg);
    void carriage_return();
    void newline();
    void backspace();
    void tab();

    // Cursor Movement
    void set_cursor_pos(int row, int col);
    void move_cursor(int delta_row, int delta_col);
    void save_cursor();
    void restore_cursor();
    int get_cursor_row() const { return cursor_.row; }
    int get_cursor_col() const { return cursor_.col; }
    bool is_cursor_visible() const { return cursor_.visible; }
    void set_cursor_visible(bool visible) { cursor_.visible = visible; }

    // Screen & Line Clearing
    void erase_in_display(int mode);
    void erase_in_line(int mode);
    void insert_lines(int count);
    void delete_lines(int count);
    void insert_blank_chars(int count);
    void delete_chars(int count);
    void erase_chars(int count);

    // Margins & Scrolling
    void set_margins(int top, int bottom);
    void scroll_up_in_region(int count);
    void scroll_down_in_region(int count);

    // Viewport Scrollback
    void scroll_viewport(int delta_lines);
    void reset_viewport_scroll();
    int get_scroll_offset() const { return scroll_offset_; }
    size_t get_scrollback_size() const { return scrollback_.size(); }

    // Alternate Screen Buffer (for vim, htop, less, etc.)
    void switch_to_alt_buffer();
    void switch_to_primary_buffer();
    bool is_alt_buffer_active() const { return using_alt_buffer_; }

    // Cell Access for Rendering
    int get_rows() const { return rows_; }
    int get_cols() const { return cols_; }
    const Cell& get_cell(int visual_row, int col) const;
    bool is_cell_selected(int visual_row, int col) const;

    // Selection & Clipboard
    void start_selection(int visual_row, int col);
    void update_selection(int visual_row, int col);
    void clear_selection();
    void select_word(int visual_row, int col);
    void select_line(int visual_row);
    int get_line_length(int visual_row) const;
    bool has_selection() const { return selection_.active; }
    std::string get_selected_text() const;

    // Modes
    bool is_bracketed_paste_mode() const { return bracketed_paste_mode_; }
    void set_bracketed_paste_mode(bool enable) { bracketed_paste_mode_ = enable; }
    bool is_app_cursor_keys() const { return app_cursor_keys_; }
    void set_app_cursor_keys(bool enable) { app_cursor_keys_ = enable; }

    void update_default_colors(const Color& fg, const Color& bg);

    // Kitty Graphics / Images
    void add_image(std::shared_ptr<ImageData> img);
    std::shared_ptr<ImageData> get_image(uint32_t id) const;
    void place_image(ImagePlacement placement);
    void delete_images(uint32_t image_id);
    void clear_all_images();
    const std::vector<ImagePlacement>& get_image_placements() const;
    int64_t get_total_lines_pushed() const { return total_lines_pushed_; }

private:
    int rows_ = 24;
    int cols_ = 80;
    Color default_fg_;
    Color default_bg_;
    size_t scrollback_max_ = 10000;

    // Grid storage
    std::vector<std::vector<Cell>> grid_;
    std::deque<std::vector<Cell>> scrollback_;
    std::vector<std::vector<Cell>> alt_grid_;

    int64_t total_lines_pushed_ = 0;
    std::unordered_map<uint32_t, std::shared_ptr<ImageData>> loaded_images_;
    std::vector<ImagePlacement> image_placements_;
    std::vector<ImagePlacement> alt_image_placements_;

    bool using_alt_buffer_ = false;
    int scroll_offset_ = 0; // 0 = live, > 0 = lines scrolled up

    CursorState cursor_;
    CursorState primary_cursor_;
    CursorState saved_primary_cursor_;
    CursorState saved_alt_cursor_;

    int top_margin_ = 0;
    int bottom_margin_ = 23;
    std::vector<bool> tab_stops_;

    Selection selection_;
    bool bracketed_paste_mode_ = false;
    bool app_cursor_keys_ = false;
    bool wrap_next_ = false;

    void init_tab_stops();
    void push_row_to_scrollback(const std::vector<Cell>& row);
    std::vector<Cell> make_blank_row() const;
};

} // namespace evaterm
