#pragma once

#include "Tab.hpp"
#include "Theme.hpp"
#include "Config.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace evaterm {

class TabManager {
public:
    TabManager(int rows, int cols, const Config& config);
    ~TabManager() = default;

    std::shared_ptr<Tab> create_tab(const std::string& shell_override = "");
    bool close_tab(size_t index);
    bool close_active_tab();

    void switch_tab(size_t index);
    void next_tab();
    void prev_tab();

    std::shared_ptr<Tab> get_active_tab();
    const std::shared_ptr<Tab> get_active_tab() const;
    std::shared_ptr<Tab> get_tab(size_t index);
    const std::shared_ptr<Tab> get_tab(size_t index) const;

    size_t get_tab_count() const { return tabs_.size(); }
    size_t get_active_index() const { return active_index_; }

    bool update_all(); // returns false if all tabs have exited
    void resize_all(int rows, int cols);
    void update_theme(const Theme& theme);

    int get_rows() const { return rows_; }
    int get_cols() const { return cols_; }

private:
    int rows_;
    int cols_;
    Config config_;
    std::vector<std::shared_ptr<Tab>> tabs_;
    size_t active_index_ = 0;
    int next_id_ = 1;
};

} // namespace evaterm
