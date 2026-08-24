#pragma once

#include "ScreenBuffer.hpp"
#include "Theme.hpp"
#include <string>
#include <vector>
#include <functional>

namespace evaterm {

enum class ParserState {
    Ground,
    Escape,
    EscapeIntermediate,
    CsiParam,
    CsiIntermediate,
    CsiIgnore,
    OscString,
    DcsEntry,
    DcsParam,
    DcsString,
    DcsIgnore
};

class AnsiParser {
public:
    using TitleCallback = std::function<void(const std::string&)>;
    using ResponseCallback = std::function<void(const std::string&)>;

    AnsiParser(ScreenBuffer& buffer, const Theme& theme);

    void parse(const char* data, size_t length);
    void reset();

    void set_title_callback(TitleCallback cb) { title_callback_ = cb; }
    void set_response_callback(ResponseCallback cb) { response_callback_ = cb; }
    void set_theme(const Theme& theme) { theme_ = theme; }

    const Color& get_current_fg() const { return current_fg_; }
    const Color& get_current_bg() const { return current_bg_; }
    uint16_t get_current_attributes() const { return current_attributes_; }

private:
    ScreenBuffer& buffer_;
    Theme theme_;

    ParserState state_ = ParserState::Ground;
    
    // Attribute state
    Color current_fg_;
    Color current_bg_;
    bool is_default_fg_ = true;
    bool is_default_bg_ = true;
    uint16_t current_attributes_ = ATTR_NONE;

    // Parameter accumulation
    std::vector<int> params_;
    int current_param_ = -1;
    bool is_private_ = false;
    char intermediate_char_ = 0;

    // OSC string accumulation
    std::string osc_string_;

    // UTF-8 decoder state
    uint32_t utf8_codepoint_ = 0;
    int utf8_bytes_remaining_ = 0;

    TitleCallback title_callback_;
    ResponseCallback response_callback_;

    void process_byte(uint8_t byte);
    void handle_ground(uint8_t byte);
    void handle_escape(uint8_t byte);
    void handle_escape_intermediate(uint8_t byte);
    void handle_csi(uint8_t byte);
    void handle_osc(uint8_t byte);

    void execute_csi_command(uint8_t final_char);
    void execute_sgr();
    void execute_osc();
    void reset_csi_params();
    void reset_sgr();

    int get_param(size_t index, int default_val) const {
        if (index < params_.size() && params_[index] >= 0) {
            return params_[index];
        }
        return default_val;
    }
};

} // namespace evaterm
