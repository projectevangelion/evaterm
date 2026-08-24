#pragma once

#include "Terminal.hpp"
#include <memory>
#include <string>

namespace evaterm {

class Tab {
public:
    Tab(int id, int rows, int cols, const Theme& theme, const std::string& shell_cmd = "", size_t scrollback_max = 10000);
    ~Tab() = default;

    bool start();
    void update();

    int get_id() const { return id_; }
    Terminal& get_terminal() { return *terminal_; }
    const Terminal& get_terminal() const { return *terminal_; }

    std::string get_title() const;
    void set_custom_title(const std::string& title) { custom_title_ = title; }

    bool is_alive() { return terminal_->is_alive(); }
    void terminate() { terminal_->terminate(); }
    void resize(int rows, int cols) { terminal_->resize(rows, cols); }

    void update_theme(const Theme& theme) { terminal_->update_theme(theme); }

private:
    int id_;
    std::unique_ptr<Terminal> terminal_;
    std::string custom_title_;
};

} // namespace evaterm
