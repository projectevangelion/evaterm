#include "TabManager.hpp"
#include <iostream>

namespace evaterm {

TabManager::TabManager(int rows, int cols, const Config& config)
    : rows_(rows), cols_(cols), config_(config) {
    create_tab();
}

std::shared_ptr<Tab> TabManager::create_tab(const std::string& shell_override) {
    std::string shell = !shell_override.empty() ? shell_override : config_.shell_path;
    auto tab = std::make_shared<Tab>(next_id_++, rows_, cols_, config_.theme, shell, config_.scrollback_limit);
    if (tab->start()) {
        tabs_.push_back(tab);
        active_index_ = tabs_.size() - 1;
        return tab;
    }
    return nullptr;
}

bool TabManager::close_tab(size_t index) {
    if (index >= tabs_.size()) return false;

    tabs_[index]->terminate();
    tabs_.erase(tabs_.begin() + index);

    if (tabs_.empty()) {
        return false; // All tabs closed
    }

    if (active_index_ >= tabs_.size()) {
        active_index_ = tabs_.size() - 1;
    }
    return true;
}

bool TabManager::close_active_tab() {
    return close_tab(active_index_);
}

void TabManager::switch_tab(size_t index) {
    if (index < tabs_.size()) {
        active_index_ = index;
    }
}

void TabManager::next_tab() {
    if (tabs_.empty()) return;
    active_index_ = (active_index_ + 1) % tabs_.size();
}

void TabManager::prev_tab() {
    if (tabs_.empty()) return;
    active_index_ = (active_index_ + tabs_.size() - 1) % tabs_.size();
}

std::shared_ptr<Tab> TabManager::get_active_tab() {
    if (active_index_ < tabs_.size()) {
        return tabs_[active_index_];
    }
    return nullptr;
}

const std::shared_ptr<Tab> TabManager::get_active_tab() const {
    if (active_index_ < tabs_.size()) {
        return tabs_[active_index_];
    }
    return nullptr;
}

std::shared_ptr<Tab> TabManager::get_tab(size_t index) {
    if (index < tabs_.size()) {
        return tabs_[index];
    }
    return nullptr;
}

const std::shared_ptr<Tab> TabManager::get_tab(size_t index) const {
    if (index < tabs_.size()) {
        return tabs_[index];
    }
    return nullptr;
}

bool TabManager::update_all() {
    // Read from all tabs so background processes keep making progress
    for (size_t i = 0; i < tabs_.size();) {
        tabs_[i]->update();
        if (!tabs_[i]->is_alive()) {
            // Tab exited
            tabs_.erase(tabs_.begin() + i);
            if (active_index_ >= tabs_.size() && !tabs_.empty()) {
                active_index_ = tabs_.size() - 1;
            }
        } else {
            ++i;
        }
    }
    return !tabs_.empty();
}

void TabManager::resize_all(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return;
    rows_ = rows;
    cols_ = cols;
    for (auto& tab : tabs_) {
        tab->resize(rows, cols);
    }
}

void TabManager::update_theme(const Theme& theme) {
    config_.theme = theme;
    for (auto& tab : tabs_) {
        tab->update_theme(theme);
    }
}

} // namespace evaterm
