#include "ScreenBuffer.hpp"
#include <iostream>

namespace evaterm {

static void utf8_append(std::string& out, char32_t cp) {
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | ((cp >> 6) & 0x1F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | ((cp >> 12) & 0x0F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | ((cp >> 18) & 0x07)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
}

ScreenBuffer::ScreenBuffer(int rows, int cols, const Color& def_fg, const Color& def_bg, size_t scrollback_max)
    : rows_(rows > 0 ? rows : 24),
      cols_(cols > 0 ? cols : 80),
      default_fg_(def_fg),
      default_bg_(def_bg),
      scrollback_max_(scrollback_max),
      top_margin_(0),
      bottom_margin_(rows_ - 1) {
    
    grid_.resize(rows_, make_blank_row());
    alt_grid_.resize(rows_, make_blank_row());
    init_tab_stops();
    cursor_.fg = def_fg;
    cursor_.bg = def_bg;
}

std::vector<Cell> ScreenBuffer::make_blank_row() const {
    std::vector<Cell> row(cols_);
    for (auto& cell : row) {
        cell.reset(default_fg_, default_bg_);
    }
    return row;
}

void ScreenBuffer::init_tab_stops() {
    tab_stops_.assign(cols_, false);
    for (int i = 8; i < cols_; i += 8) {
        tab_stops_[i] = true;
    }
}

void ScreenBuffer::reset() {
    grid_.assign(rows_, make_blank_row());
    alt_grid_.assign(rows_, make_blank_row());
    scrollback_.clear();
    scroll_offset_ = 0;
    using_alt_buffer_ = false;
    top_margin_ = 0;
    bottom_margin_ = rows_ - 1;
    cursor_ = CursorState{};
    cursor_.fg = default_fg_;
    cursor_.bg = default_bg_;
    primary_cursor_ = cursor_;
    saved_primary_cursor_ = cursor_;
    saved_alt_cursor_ = cursor_;
    wrap_next_ = false;
    clear_selection();
}

void ScreenBuffer::resize(int new_rows, int new_cols) {
    if (new_rows <= 0 || new_cols <= 0) return;
    if (new_rows == rows_ && new_cols == cols_) return;

    std::vector<std::vector<Cell>> new_grid(new_rows, std::vector<Cell>(new_cols));
    for (int r = 0; r < new_rows; ++r) {
        for (int c = 0; c < new_cols; ++c) {
            if (r < rows_ && c < cols_) {
                new_grid[r][c] = grid_[r][c];
            } else {
                new_grid[r][c].reset(default_fg_, default_bg_);
            }
        }
    }

    std::vector<std::vector<Cell>> new_alt(new_rows, std::vector<Cell>(new_cols));
    for (int r = 0; r < new_rows; ++r) {
        for (int c = 0; c < new_cols; ++c) {
            if (r < rows_ && c < cols_) {
                new_alt[r][c] = alt_grid_[r][c];
            } else {
                new_alt[r][c].reset(default_fg_, default_bg_);
            }
        }
    }

    grid_ = std::move(new_grid);
    alt_grid_ = std::move(new_alt);

    rows_ = new_rows;
    cols_ = new_cols;
    top_margin_ = 0;
    bottom_margin_ = rows_ - 1;
    init_tab_stops();

    cursor_.row = std::clamp(cursor_.row, 0, rows_ - 1);
    cursor_.col = std::clamp(cursor_.col, 0, cols_ - 1);
    wrap_next_ = false;
}

void ScreenBuffer::push_row_to_scrollback(const std::vector<Cell>& row) {
    if (using_alt_buffer_ || scrollback_max_ == 0) return;
    scrollback_.push_back(row);
    if (scrollback_.size() > scrollback_max_) {
        scrollback_.pop_front();
    }
}

void ScreenBuffer::write_char(char32_t codepoint, const Color& fg, const Color& bg, uint16_t attrs, bool def_fg, bool def_bg) {
    if (wrap_next_) {
        cursor_.col = 0;
        if (cursor_.row >= bottom_margin_) {
            scroll_up_in_region(1);
        } else {
            cursor_.row++;
        }
        wrap_next_ = false;
    }

    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
    if (cursor_.row >= 0 && cursor_.row < rows_ && cursor_.col >= 0 && cursor_.col < cols_) {
        Cell& cell = active_grid[cursor_.row][cursor_.col];
        cell.codepoint = codepoint;
        cell.fg = fg;
        cell.bg = bg;
        cell.attributes = attrs;
        cell.is_default_fg = def_fg;
        cell.is_default_bg = def_bg;
    }

    if (cursor_.col + 1 >= cols_) {
        wrap_next_ = true;
    } else {
        cursor_.col++;
    }

    // New output automatically returns viewport to live view
    if (scroll_offset_ > 0) {
        scroll_offset_ = 0;
    }
}

void ScreenBuffer::carriage_return() {
    cursor_.col = 0;
    wrap_next_ = false;
}

void ScreenBuffer::newline() {
    wrap_next_ = false;
    if (cursor_.row == bottom_margin_) {
        scroll_up_in_region(1);
    } else if (cursor_.row < rows_ - 1) {
        cursor_.row++;
    }
    if (scroll_offset_ > 0) {
        scroll_offset_ = 0;
    }
}

void ScreenBuffer::backspace() {
    wrap_next_ = false;
    if (cursor_.col > 0) {
        cursor_.col--;
    }
}

void ScreenBuffer::tab() {
    wrap_next_ = false;
    for (int c = cursor_.col + 1; c < cols_; ++c) {
        if (tab_stops_[c]) {
            cursor_.col = c;
            return;
        }
    }
    cursor_.col = cols_ - 1;
}

void ScreenBuffer::set_cursor_pos(int row, int col) {
    cursor_.row = std::clamp(row, 0, rows_ - 1);
    cursor_.col = std::clamp(col, 0, cols_ - 1);
    wrap_next_ = false;
}

void ScreenBuffer::move_cursor(int delta_row, int delta_col) {
    set_cursor_pos(cursor_.row + delta_row, cursor_.col + delta_col);
}

void ScreenBuffer::save_cursor() {
    if (using_alt_buffer_) {
        saved_alt_cursor_ = cursor_;
    } else {
        saved_primary_cursor_ = cursor_;
    }
}

void ScreenBuffer::restore_cursor() {
    if (using_alt_buffer_) {
        cursor_ = saved_alt_cursor_;
    } else {
        cursor_ = saved_primary_cursor_;
    }
    cursor_.row = std::clamp(cursor_.row, 0, rows_ - 1);
    cursor_.col = std::clamp(cursor_.col, 0, cols_ - 1);
    wrap_next_ = false;
}

void ScreenBuffer::erase_in_display(int mode) {
    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;

    if (mode == 0) { // Cursor to end of screen
        for (int c = cursor_.col; c < cols_; ++c) {
            active_grid[cursor_.row][c].reset(default_fg_, default_bg_);
        }
        for (int r = cursor_.row + 1; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                active_grid[r][c].reset(default_fg_, default_bg_);
            }
        }
    } else if (mode == 1) { // Start of screen to cursor
        for (int r = 0; r < cursor_.row; ++r) {
            for (int c = 0; c < cols_; ++c) {
                active_grid[r][c].reset(default_fg_, default_bg_);
            }
        }
        for (int c = 0; c <= cursor_.col && c < cols_; ++c) {
            active_grid[cursor_.row][c].reset(default_fg_, default_bg_);
        }
    } else if (mode == 2 || mode == 3) { // Entire screen (3 clears scrollback too)
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                active_grid[r][c].reset(default_fg_, default_bg_);
            }
        }
        if (mode == 3 && !using_alt_buffer_) {
            scrollback_.clear();
            scroll_offset_ = 0;
        }
    }
}

void ScreenBuffer::erase_in_line(int mode) {
    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
    if (cursor_.row < 0 || cursor_.row >= rows_) return;

    if (mode == 0) { // Cursor to end of line
        for (int c = cursor_.col; c < cols_; ++c) {
            active_grid[cursor_.row][c].reset(default_fg_, default_bg_);
        }
    } else if (mode == 1) { // Start of line to cursor
        for (int c = 0; c <= cursor_.col && c < cols_; ++c) {
            active_grid[cursor_.row][c].reset(default_fg_, default_bg_);
        }
    } else if (mode == 2) { // Entire line
        for (int c = 0; c < cols_; ++c) {
            active_grid[cursor_.row][c].reset(default_fg_, default_bg_);
        }
    }
}

void ScreenBuffer::insert_blank_chars(int count) {
    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
    if (cursor_.row < 0 || cursor_.row >= rows_) return;
    int r = cursor_.row;
    count = std::min(count, cols_ - cursor_.col);

    for (int c = cols_ - 1; c >= cursor_.col + count; --c) {
        active_grid[r][c] = active_grid[r][c - count];
    }
    for (int c = cursor_.col; c < cursor_.col + count; ++c) {
        active_grid[r][c].reset(default_fg_, default_bg_);
    }
}

void ScreenBuffer::delete_chars(int count) {
    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
    if (cursor_.row < 0 || cursor_.row >= rows_) return;
    int r = cursor_.row;
    count = std::min(count, cols_ - cursor_.col);

    for (int c = cursor_.col; c + count < cols_; ++c) {
        active_grid[r][c] = active_grid[r][c + count];
    }
    for (int c = cols_ - count; c < cols_; ++c) {
        active_grid[r][c].reset(default_fg_, default_bg_);
    }
}

void ScreenBuffer::erase_chars(int count) {
    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
    if (cursor_.row < 0 || cursor_.row >= rows_) return;
    int r = cursor_.row;
    int end_col = std::min(cols_, cursor_.col + count);

    for (int c = cursor_.col; c < end_col; ++c) {
        active_grid[r][c].reset(default_fg_, default_bg_);
    }
}

void ScreenBuffer::set_margins(int top, int bottom) {
    top_margin_ = std::clamp(top, 0, rows_ - 1);
    bottom_margin_ = std::clamp(bottom, top_margin_, rows_ - 1);
}

void ScreenBuffer::scroll_up_in_region(int count) {
    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
    count = std::clamp(count, 1, bottom_margin_ - top_margin_ + 1);

    for (int i = 0; i < count; ++i) {
        if (top_margin_ == 0) {
            push_row_to_scrollback(active_grid[0]);
        }
        for (int r = top_margin_; r < bottom_margin_; ++r) {
            active_grid[r] = std::move(active_grid[r + 1]);
        }
        active_grid[bottom_margin_] = make_blank_row();
    }
}

void ScreenBuffer::scroll_down_in_region(int count) {
    auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
    count = std::clamp(count, 1, bottom_margin_ - top_margin_ + 1);

    for (int i = 0; i < count; ++i) {
        for (int r = bottom_margin_; r > top_margin_; --r) {
            active_grid[r] = std::move(active_grid[r - 1]);
        }
        active_grid[top_margin_] = make_blank_row();
    }
}

void ScreenBuffer::insert_lines(int count) {
    if (cursor_.row < top_margin_ || cursor_.row > bottom_margin_) return;
    int old_top = top_margin_;
    top_margin_ = cursor_.row;
    scroll_down_in_region(count);
    top_margin_ = old_top;
}

void ScreenBuffer::delete_lines(int count) {
    if (cursor_.row < top_margin_ || cursor_.row > bottom_margin_) return;
    int old_top = top_margin_;
    top_margin_ = cursor_.row;
    scroll_up_in_region(count);
    top_margin_ = old_top;
}

void ScreenBuffer::scroll_viewport(int delta_lines) {
    if (using_alt_buffer_) return; // Alt buffer doesn't scroll viewport
    int max_scroll = static_cast<int>(scrollback_.size());
    scroll_offset_ = std::clamp(scroll_offset_ + delta_lines, 0, max_scroll);
}

void ScreenBuffer::reset_viewport_scroll() {
    scroll_offset_ = 0;
}

void ScreenBuffer::switch_to_alt_buffer() {
    if (!using_alt_buffer_) {
        primary_cursor_ = cursor_;
        saved_primary_cursor_ = cursor_;
        using_alt_buffer_ = true;
        alt_grid_.assign(rows_, make_blank_row());
        cursor_ = CursorState{};
        cursor_.fg = default_fg_;
        cursor_.bg = default_bg_;
        scroll_offset_ = 0;
    }
}

void ScreenBuffer::switch_to_primary_buffer() {
    if (using_alt_buffer_) {
        using_alt_buffer_ = false;
        cursor_ = primary_cursor_;
        scroll_offset_ = 0;
    }
}

const Cell& ScreenBuffer::get_cell(int visual_row, int col) const {
    static const Cell blank_cell;
    if (visual_row < 0 || visual_row >= rows_ || col < 0 || col >= cols_) {
        return blank_cell;
    }

    if (using_alt_buffer_ || scroll_offset_ == 0) {
        const auto& active_grid = using_alt_buffer_ ? alt_grid_ : grid_;
        return active_grid[visual_row][col];
    }

    // Viewing history with scroll_offset_ > 0
    int history_index = static_cast<int>(scrollback_.size()) - scroll_offset_ + visual_row;
    if (history_index >= 0 && history_index < static_cast<int>(scrollback_.size())) {
        return scrollback_[history_index][col];
    }

    int grid_index = history_index - static_cast<int>(scrollback_.size());
    if (grid_index >= 0 && grid_index < rows_) {
        return grid_[grid_index][col];
    }

    return blank_cell;
}

int ScreenBuffer::get_line_length(int visual_row) const {
    if (visual_row < 0 || visual_row >= rows_) return 0;
    for (int c = cols_ - 1; c >= 0; --c) {
        const Cell& cell = get_cell(visual_row, c);
        if (cell.codepoint != ' ' && cell.codepoint != 0) {
            return c + 1;
        }
    }
    return 0;
}

static bool is_word_character(char32_t cp) {
    if (cp == 0 || cp == ' ' || cp == '\t' || cp == '\n') return false;
    if (cp == '"' || cp == '\'' || cp == '`' || cp == '(' || cp == ')' ||
        cp == '[' || cp == ']' || cp == '{' || cp == '}' || cp == '<' ||
        cp == '>' || cp == '=' || cp == ';' || cp == ',' || cp == '|') {
        return false;
    }
    return true;
}

void ScreenBuffer::select_word(int visual_row, int col) {
    if (visual_row < 0 || visual_row >= rows_) return;
    int line_len = get_line_length(visual_row);
    if (line_len == 0 || col >= line_len) {
        clear_selection();
        return;
    }

    char32_t target_cp = get_cell(visual_row, col).codepoint;
    bool target_is_word = is_word_character(target_cp);

    int start_col = col;
    while (start_col > 0) {
        char32_t prev_cp = get_cell(visual_row, start_col - 1).codepoint;
        if (target_is_word && is_word_character(prev_cp)) {
            start_col--;
        } else if (!target_is_word && !is_word_character(prev_cp) && prev_cp != ' ' && prev_cp != 0) {
            start_col--;
        } else {
            break;
        }
    }

    int end_col = col;
    while (end_col + 1 < line_len) {
        char32_t next_cp = get_cell(visual_row, end_col + 1).codepoint;
        if (target_is_word && is_word_character(next_cp)) {
            end_col++;
        } else if (!target_is_word && !is_word_character(next_cp) && next_cp != ' ' && next_cp != 0) {
            end_col++;
        } else {
            break;
        }
    }

    selection_.active = true;
    selection_.start_row = visual_row;
    selection_.start_col = start_col;
    selection_.end_row = visual_row;
    selection_.end_col = end_col;
}

void ScreenBuffer::select_line(int visual_row) {
    if (visual_row < 0 || visual_row >= rows_) return;
    int line_len = get_line_length(visual_row);
    if (line_len == 0) {
        clear_selection();
        return;
    }

    selection_.active = true;
    selection_.start_row = visual_row;
    selection_.start_col = 0;
    selection_.end_row = visual_row;
    selection_.end_col = line_len - 1;
}

void ScreenBuffer::start_selection(int visual_row, int col) {
    if (visual_row < 0 || visual_row >= rows_) {
        clear_selection();
        return;
    }
    int line_len = get_line_length(visual_row);
    if (line_len == 0 || col >= line_len) {
        clear_selection();
        return;
    }

    selection_.active = true;
    selection_.start_row = visual_row;
    selection_.start_col = col;
    selection_.end_row = visual_row;
    selection_.end_col = col;
}

void ScreenBuffer::update_selection(int visual_row, int col) {
    if (visual_row < 0 || visual_row >= rows_) return;
    
    if (!selection_.active) {
        int line_len = get_line_length(visual_row);
        if (line_len > 0 && col < line_len) {
            selection_.active = true;
            selection_.start_row = visual_row;
            selection_.start_col = col;
        } else {
            return;
        }
    }

    int line_len = get_line_length(visual_row);
    int clamped_col = (line_len > 0) ? std::min(col, line_len - 1) : 0;

    selection_.end_row = visual_row;
    selection_.end_col = clamped_col;
}

void ScreenBuffer::clear_selection() {
    selection_.active = false;
}

bool ScreenBuffer::is_cell_selected(int visual_row, int col) const {
    if (!selection_.active) return false;
    int r0, c0, r1, c1;
    selection_.normalize(r0, c0, r1, c1);

    if (visual_row < r0 || visual_row > r1) return false;

    int line_len = get_line_length(visual_row);
    if (line_len == 0 || col >= line_len) return false; // Do not highlight empty space

    if (visual_row == r0 && visual_row == r1) {
        return col >= c0 && col <= c1;
    }
    if (visual_row == r0) {
        return col >= c0;
    }
    if (visual_row == r1) {
        return col <= c1;
    }
    // Full line in between
    return true;
}

std::string ScreenBuffer::get_selected_text() const {
    if (!selection_.active) return "";
    int r0, c0, r1, c1;
    selection_.normalize(r0, c0, r1, c1);

    std::string text;
    bool has_text = false;

    for (int r = r0; r <= r1; ++r) {
        int line_len = get_line_length(r);
        if (line_len == 0) {
            if (r < r1 && has_text) {
                text.push_back('\n');
            }
            continue;
        }

        int col_start = (r == r0) ? c0 : 0;
        int col_end = (r == r1) ? std::min(c1, line_len - 1) : (line_len - 1);

        if (col_start <= col_end && col_start < line_len) {
            has_text = true;
            for (int c = col_start; c <= col_end; ++c) {
                const Cell& cell = get_cell(r, c);
                if (cell.codepoint != 0) {
                    utf8_append(text, cell.codepoint);
                } else {
                    text.push_back(' ');
                }
            }
        }

        if (r < r1 && has_text) {
            text.push_back('\n');
        }
    }

    while (!text.empty() && text.back() == ' ') {
        text.pop_back();
    }
    return text;
}

void ScreenBuffer::update_default_colors(const Color& fg, const Color& bg) {
    default_fg_ = fg;
    default_bg_ = bg;
}

} // namespace evaterm
