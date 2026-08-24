#include "Terminal.hpp"
#include <iostream>

namespace evaterm {

Terminal::Terminal(int rows, int cols, const Theme& theme, const std::string& shell_cmd, size_t scrollback_max)
    : rows_(rows),
      cols_(cols),
      theme_(theme),
      shell_cmd_(shell_cmd),
      scrollback_max_(scrollback_max),
      buffer_(rows, cols, theme.foreground, theme.background, scrollback_max),
      parser_(buffer_, theme) {

    parser_.set_title_callback([this](const std::string& new_title) {
        title_ = new_title;
    });

    parser_.set_response_callback([this](const std::string& response) {
        pty_.write_string(response);
    });
}

bool Terminal::start() {
    return pty_.start(rows_, cols_, shell_cmd_);
}

void Terminal::update() {
    // Read non-blocking data from PTY
    while (true) {
        ssize_t bytes_read = pty_.read_bytes(read_buffer_, sizeof(read_buffer_));
        if (bytes_read > 0) {
            parser_.parse(read_buffer_, static_cast<size_t>(bytes_read));
        } else {
            break; // 0 = no data or EOF
        }
    }
}

void Terminal::write_input(const char* data, size_t length) {
    pty_.write_bytes(data, length);
    // Writing input resets scrollback offset to live
    buffer_.reset_viewport_scroll();
}

void Terminal::write_input(const std::string& data) {
    write_input(data.data(), data.size());
}

void Terminal::resize(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return;
    if (rows == rows_ && cols == cols_) return;
    rows_ = rows;
    cols_ = cols;
    buffer_.resize(rows, cols);
    pty_.resize(rows, cols);
}

std::string Terminal::get_display_title() const {
    if (!title_.empty()) {
        return title_;
    }
    return pty_.get_active_process_name();
}

void Terminal::update_theme(const Theme& theme) {
    theme_ = theme;
    buffer_.update_default_colors(theme.foreground, theme.background);
    parser_.set_theme(theme);
}

} // namespace evaterm
