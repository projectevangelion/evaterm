#pragma once

#include "Pty.hpp"
#include "ScreenBuffer.hpp"
#include "AnsiParser.hpp"
#include "Theme.hpp"
#include <string>
#include <memory>

namespace evaterm {

class Terminal {
public:
    Terminal(int rows, int cols, const Theme& theme, const std::string& shell_cmd = "", size_t scrollback_max = 10000);
    ~Terminal() = default;

    bool start();
    void update(); // Non-blocking read from PTY
    void write_input(const char* data, size_t length);
    void write_input(const std::string& data);
    void resize(int rows, int cols);

    ScreenBuffer& get_buffer() { return buffer_; }
    const ScreenBuffer& get_buffer() const { return buffer_; }

    Pty& get_pty() { return pty_; }
    const Pty& get_pty() const { return pty_; }

    bool is_alive() { return pty_.is_alive(); }
    void terminate() { pty_.terminate(); }

    const std::string& get_title() const { return title_; }
    void set_title(const std::string& title) { title_ = title; }
    std::string get_display_title() const;

    void update_theme(const Theme& theme);

private:
    int rows_;
    int cols_;
    Theme theme_;
    std::string shell_cmd_;
    size_t scrollback_max_;

    ScreenBuffer buffer_;
    AnsiParser parser_;
    Pty pty_;

    std::string title_;
    char read_buffer_[8192];
};

} // namespace evaterm
