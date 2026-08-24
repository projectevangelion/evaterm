#include "Tab.hpp"

namespace evaterm {

Tab::Tab(int id, int rows, int cols, const Theme& theme, const std::string& shell_cmd, size_t scrollback_max)
    : id_(id),
      terminal_(std::make_unique<Terminal>(rows, cols, theme, shell_cmd, scrollback_max)) {}

bool Tab::start() {
    return terminal_->start();
}

void Tab::update() {
    terminal_->update();
}

std::string Tab::get_title() const {
    if (!custom_title_.empty()) {
        return custom_title_;
    }
    return terminal_->get_display_title();
}

} // namespace evaterm
