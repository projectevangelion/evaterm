#include "AnsiParser.hpp"
#include <iostream>
#include <sstream>
#include <fstream>

namespace evaterm {

AnsiParser::AnsiParser(ScreenBuffer& buffer, const Theme& theme)
    : buffer_(buffer), theme_(theme) {
    reset_sgr();
}

void AnsiParser::reset() {
    state_ = ParserState::Ground;
    reset_csi_params();
    reset_sgr();
    osc_string_.clear();
    apc_string_.clear();
    kitty_chunk_payload_.clear();
    kitty_active_params_.clear();
    utf8_codepoint_ = 0;
    utf8_bytes_remaining_ = 0;
}

void AnsiParser::reset_sgr() {
    current_fg_ = theme_.foreground;
    current_bg_ = theme_.background;
    is_default_fg_ = true;
    is_default_bg_ = true;
    current_attributes_ = ATTR_NONE;
}

void AnsiParser::reset_csi_params() {
    params_.clear();
    current_param_ = -1;
    is_private_ = false;
    intermediate_char_ = 0;
}

void AnsiParser::parse(const char* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        process_byte(static_cast<uint8_t>(data[i]));
    }
}

void AnsiParser::process_byte(uint8_t byte) {
    // Check for global cancellation/escape sequences
    if (byte == 0x1B) { // ESC always transitions to Escape state (except inside string terminator)
        if (state_ == ParserState::ApcString) {
            handle_apc(byte);
            return;
        }
        state_ = ParserState::Escape;
        intermediate_char_ = 0;
        return;
    }

    if (byte == 0x18 || byte == 0x1A) { // CAN, SUB
        state_ = ParserState::Ground;
        return;
    }

    switch (state_) {
        case ParserState::Ground:
            handle_ground(byte);
            break;
        case ParserState::Escape:
            handle_escape(byte);
            break;
        case ParserState::EscapeIntermediate:
            handle_escape_intermediate(byte);
            break;
        case ParserState::CsiParam:
            handle_csi(byte);
            break;
        case ParserState::OscString:
            handle_osc(byte);
            break;
        case ParserState::ApcString:
            handle_apc(byte);
            break;
        default:
            state_ = ParserState::Ground;
            handle_ground(byte);
            break;
    }
}

void AnsiParser::handle_ground(uint8_t byte) {
    // Control characters
    if (byte == '\r') {
        buffer_.carriage_return();
        return;
    }
    if (byte == '\n' || byte == 0x0B || byte == 0x0C) { // LF, VT, FF
        buffer_.newline();
        return;
    }
    if (byte == '\b') {
        buffer_.backspace();
        return;
    }
    if (byte == '\t') {
        buffer_.tab();
        return;
    }
    if (byte == 0x07) { // BEL
        // Audible or visual bell
        return;
    }
    if (byte < 0x20) {
        // Ignore other non-printable ASCII control codes in ground state
        return;
    }

    // UTF-8 decoding
    if (utf8_bytes_remaining_ == 0) {
        if ((byte & 0x80) == 0) {
            // 1-byte ASCII
            buffer_.write_char(static_cast<char32_t>(byte), current_fg_, current_bg_, current_attributes_, is_default_fg_, is_default_bg_);
        } else if ((byte & 0xE0) == 0xC0) {
            utf8_codepoint_ = byte & 0x1F;
            utf8_bytes_remaining_ = 1;
        } else if ((byte & 0xF0) == 0xE0) {
            utf8_codepoint_ = byte & 0x0F;
            utf8_bytes_remaining_ = 2;
        } else if ((byte & 0xF8) == 0xF0) {
            utf8_codepoint_ = byte & 0x07;
            utf8_bytes_remaining_ = 3;
        }
    } else {
        if ((byte & 0xC0) == 0x80) {
            utf8_codepoint_ = (utf8_codepoint_ << 6) | (byte & 0x3F);
            utf8_bytes_remaining_--;
            if (utf8_bytes_remaining_ == 0) {
                buffer_.write_char(static_cast<char32_t>(utf8_codepoint_), current_fg_, current_bg_, current_attributes_, is_default_fg_, is_default_bg_);
            }
        } else {
            // Invalid UTF-8 sequence, reset
            utf8_bytes_remaining_ = 0;
            utf8_codepoint_ = 0;
        }
    }
}

void AnsiParser::handle_escape(uint8_t byte) {
    if (byte == '[') {
        state_ = ParserState::CsiParam;
        reset_csi_params();
    } else if (byte == ']') {
        state_ = ParserState::OscString;
        osc_string_.clear();
    } else if (byte == '_') { // APC (Kitty Graphics Protocol)
        state_ = ParserState::ApcString;
        apc_string_.clear();
    } else if (byte == '(' || byte == ')' || byte == '*' || byte == '+' || byte == '-' || byte == '.' || byte == '/' || byte == '#' || byte == '%' || byte == ' ') {
        // Multi-byte escape sequence intermediate (e.g. \033(B)
        intermediate_char_ = static_cast<char>(byte);
        state_ = ParserState::EscapeIntermediate;
    } else if (byte == '7') { // Save cursor
        buffer_.save_cursor();
        state_ = ParserState::Ground;
    } else if (byte == '8') { // Restore cursor
        buffer_.restore_cursor();
        state_ = ParserState::Ground;
    } else if (byte == 'c') { // Full reset (RIS)
        buffer_.reset();
        reset_sgr();
        state_ = ParserState::Ground;
    } else if (byte == 'D') { // Index (IND)
        buffer_.newline();
        state_ = ParserState::Ground;
    } else if (byte == 'E') { // Next Line (NEL)
        buffer_.carriage_return();
        buffer_.newline();
        state_ = ParserState::Ground;
    } else if (byte == 'M') { // Reverse index (scroll down / move up)
        if (buffer_.get_cursor_row() == 0) {
            buffer_.scroll_down_in_region(1);
        } else {
            buffer_.move_cursor(-1, 0);
        }
        state_ = ParserState::Ground;
    } else {
        state_ = ParserState::Ground;
    }
}

void AnsiParser::handle_escape_intermediate(uint8_t byte) {
    // Absorbs the designated charset byte (e.g. 'B' in \033(B)
    state_ = ParserState::Ground;
}

void AnsiParser::handle_csi(uint8_t byte) {
    if (byte >= '0' && byte <= '9') {
        if (current_param_ < 0) current_param_ = 0;
        current_param_ = current_param_ * 10 + (byte - '0');
    } else if (byte == ';') {
        params_.push_back(current_param_);
        current_param_ = -1;
    } else if (byte == '?') {
        is_private_ = true;
    } else if (byte >= 0x40 && byte <= 0x7E) {
        // Final command character
        if (current_param_ >= 0 || !params_.empty()) {
            params_.push_back(current_param_);
        }
        execute_csi_command(byte);
        state_ = ParserState::Ground;
    } else {
        // Intermediate or unrecognized CSI byte
        intermediate_char_ = static_cast<char>(byte);
    }
}

void AnsiParser::execute_csi_command(uint8_t final_char) {
    if (is_private_) {
        // DEC Private Mode (e.g. ?25h / ?25l, ?1049h / ?1049l)
        int mode = get_param(0, 0);
        if (final_char == 'h') { // Set mode
            if (mode == 25) {
                buffer_.set_cursor_visible(true);
            } else if (mode == 1049 || mode == 47 || mode == 1047) {
                buffer_.switch_to_alt_buffer();
            } else if (mode == 1048) {
                buffer_.save_cursor();
            } else if (mode == 2004) {
                buffer_.set_bracketed_paste_mode(true);
            } else if (mode == 1) {
                buffer_.set_app_cursor_keys(true);
            }
        } else if (final_char == 'l') { // Reset mode
            if (mode == 25) {
                buffer_.set_cursor_visible(false);
            } else if (mode == 1049 || mode == 47 || mode == 1047) {
                buffer_.switch_to_primary_buffer();
            } else if (mode == 1048) {
                buffer_.restore_cursor();
            } else if (mode == 2004) {
                buffer_.set_bracketed_paste_mode(false);
            } else if (mode == 1) {
                buffer_.set_app_cursor_keys(false);
            }
        }
        return;
    }

    switch (final_char) {
        case 'A': { // Cursor Up (CUU)
            int n = get_param(0, 1);
            buffer_.move_cursor(-n, 0);
            break;
        }
        case 'B': { // Cursor Down (CUD)
            int n = get_param(0, 1);
            buffer_.move_cursor(n, 0);
            break;
        }
        case 'C': { // Cursor Forward (CUF)
            int n = get_param(0, 1);
            buffer_.move_cursor(0, n);
            break;
        }
        case 'D': { // Cursor Back (CUB)
            int n = get_param(0, 1);
            buffer_.move_cursor(0, -n);
            break;
        }
        case 'E': { // Cursor Next Line (CNL)
            int n = get_param(0, 1);
            buffer_.set_cursor_pos(buffer_.get_cursor_row() + n, 0);
            break;
        }
        case 'F': { // Cursor Previous Line (CPL)
            int n = get_param(0, 1);
            buffer_.set_cursor_pos(buffer_.get_cursor_row() - n, 0);
            break;
        }
        case 'G': { // Cursor Horizontal Absolute (CHA)
            int col = get_param(0, 1) - 1;
            buffer_.set_cursor_pos(buffer_.get_cursor_row(), col);
            break;
        }
        case 'H':   // Cursor Position (CUP)
        case 'f': { // Horizontal and Vertical Position (HVP)
            int row = get_param(0, 1) - 1;
            int col = get_param(1, 1) - 1;
            buffer_.set_cursor_pos(row, col);
            break;
        }
        case 'J': { // Erase in Display (ED)
            int mode = get_param(0, 0);
            buffer_.erase_in_display(mode);
            break;
        }
        case 'K': { // Erase in Line (EL)
            int mode = get_param(0, 0);
            buffer_.erase_in_line(mode);
            break;
        }
        case 'L': { // Insert Line (IL)
            int n = get_param(0, 1);
            buffer_.insert_lines(n);
            break;
        }
        case 'M': { // Delete Line (DL)
            int n = get_param(0, 1);
            buffer_.delete_lines(n);
            break;
        }
        case 'P': { // Delete Character (DCH)
            int n = get_param(0, 1);
            buffer_.delete_chars(n);
            break;
        }
        case '@': { // Insert Blank Character (ICH)
            int n = get_param(0, 1);
            buffer_.insert_blank_chars(n);
            break;
        }
        case 'X': { // Erase Character (ECH)
            int n = get_param(0, 1);
            buffer_.erase_chars(n);
            break;
        }
        case 'S': { // Scroll Up (SU)
            int n = get_param(0, 1);
            buffer_.scroll_up_in_region(n);
            break;
        }
        case 'T': { // Scroll Down (SD)
            int n = get_param(0, 1);
            buffer_.scroll_down_in_region(n);
            break;
        }
        case 'd': { // Line Position Absolute (VPA)
            int row = get_param(0, 1) - 1;
            buffer_.set_cursor_pos(row, buffer_.get_cursor_col());
            break;
        }
        case 'r': { // Set Top and Bottom Margins (DECSTBM)
            int top = get_param(0, 1) - 1;
            int bottom = get_param(1, buffer_.get_rows()) - 1;
            buffer_.set_margins(top, bottom);
            buffer_.set_cursor_pos(0, 0);
            break;
        }
        case 's': { // Save Cursor Position (SCO)
            buffer_.save_cursor();
            break;
        }
        case 'u': { // Restore Cursor Position (SCO)
            buffer_.restore_cursor();
            break;
        }
        case 'm': { // Select Graphic Rendition (SGR)
            execute_sgr();
            break;
        }
        case 'n': { // Device Status Report (DSR)
            int cmd = get_param(0, 0);
            if (cmd == 6 && response_callback_) { // Cursor position query
                std::string resp = "\033[" + std::to_string(buffer_.get_cursor_row() + 1) + ";" +
                                   std::to_string(buffer_.get_cursor_col() + 1) + "R";
                response_callback_(resp);
            }
            break;
        }
        case 'c': { // Primary Device Attributes
            if (response_callback_) {
                response_callback_("\033[?6c"); // VT102 compatible
            }
            break;
        }
        default:
            break;
    }
}

void AnsiParser::execute_sgr() {
    if (params_.empty()) {
        reset_sgr();
        return;
    }

    for (size_t i = 0; i < params_.size(); ++i) {
        int code = params_[i];
        if (code < 0 || code == 0) {
            reset_sgr();
        } else if (code == 1) {
            current_attributes_ |= ATTR_BOLD;
        } else if (code == 2) {
            current_attributes_ |= ATTR_DIM;
        } else if (code == 3) {
            current_attributes_ |= ATTR_ITALIC;
        } else if (code == 4) {
            current_attributes_ |= ATTR_UNDERLINE;
        } else if (code == 5) {
            current_attributes_ |= ATTR_BLINK;
        } else if (code == 7) {
            current_attributes_ |= ATTR_INVERSE;
        } else if (code == 8) {
            current_attributes_ |= ATTR_HIDDEN;
        } else if (code == 9) {
            current_attributes_ |= ATTR_STRIKETHROUGH;
        } else if (code == 22) {
            current_attributes_ &= ~(ATTR_BOLD | ATTR_DIM);
        } else if (code == 23) {
            current_attributes_ &= ~ATTR_ITALIC;
        } else if (code == 24) {
            current_attributes_ &= ~ATTR_UNDERLINE;
        } else if (code == 25) {
            current_attributes_ &= ~ATTR_BLINK;
        } else if (code == 27) {
            current_attributes_ &= ~ATTR_INVERSE;
        } else if (code == 28) {
            current_attributes_ &= ~ATTR_HIDDEN;
        } else if (code == 29) {
            current_attributes_ &= ~ATTR_STRIKETHROUGH;
        } else if (code >= 30 && code <= 37) { // Standard 8 FG colors
            current_fg_ = theme_.ansi_colors[code - 30];
            is_default_fg_ = false;
        } else if (code == 39) { // Default FG
            current_fg_ = theme_.foreground;
            is_default_fg_ = true;
        } else if (code >= 40 && code <= 47) { // Standard 8 BG colors
            current_bg_ = theme_.ansi_colors[code - 40];
            is_default_bg_ = false;
        } else if (code == 49) { // Default BG
            current_bg_ = theme_.background;
            is_default_bg_ = true;
        } else if (code >= 90 && code <= 97) { // Bright 8 FG colors
            current_fg_ = theme_.ansi_colors[8 + (code - 90)];
            is_default_fg_ = false;
        } else if (code >= 100 && code <= 107) { // Bright 8 BG colors
            current_bg_ = theme_.ansi_colors[8 + (code - 100)];
            is_default_bg_ = false;
        } else if (code == 38 || code == 48) { // Extended 256 / TrueColor
            bool is_fg = (code == 38);
            if (i + 1 < params_.size()) {
                int mode = params_[i + 1];
                if (mode == 5 && i + 2 < params_.size()) { // 256 colors
                    uint8_t color_idx = static_cast<uint8_t>(params_[i + 2]);
                    Color col = theme_.get_256_color(color_idx);
                    if (is_fg) { current_fg_ = col; is_default_fg_ = false; }
                    else { current_bg_ = col; is_default_bg_ = false; }
                    i += 2;
                } else if (mode == 2 && i + 4 < params_.size()) { // 24-bit TrueColor RGB
                    uint8_t r = static_cast<uint8_t>(params_[i + 2]);
                    uint8_t g = static_cast<uint8_t>(params_[i + 3]);
                    uint8_t b = static_cast<uint8_t>(params_[i + 4]);
                    Color col(r, g, b);
                    if (is_fg) { current_fg_ = col; is_default_fg_ = false; }
                    else { current_bg_ = col; is_default_bg_ = false; }
                    i += 4;
                }
            }
        }
    }
}

void AnsiParser::handle_osc(uint8_t byte) {
    if (byte == 0x07 || byte == 0x1B) { // BEL or start of ESC \ string terminator
        execute_osc();
        state_ = ParserState::Ground;
    } else {
        osc_string_.push_back(static_cast<char>(byte));
    }
}

void AnsiParser::execute_osc() {
    if (osc_string_.empty()) return;

    size_t semicolon = osc_string_.find(';');
    if (semicolon != std::string::npos) {
        std::string cmd_str = osc_string_.substr(0, semicolon);
        std::string payload = osc_string_.substr(semicolon + 1);

        try {
            int cmd = std::stoi(cmd_str);
            if ((cmd == 0 || cmd == 2) && title_callback_) {
                // Set window / tab title
                title_callback_(payload);
            }
        } catch (...) {}
    }
}

void AnsiParser::handle_apc(uint8_t byte) {
    if (byte == 0x07) { // BEL terminates APC
        execute_kitty_graphics();
        state_ = ParserState::Ground;
        return;
    }
    if (byte == 0x1B) { // ESC (may start ST \033\)
        apc_string_.push_back(static_cast<char>(byte));
        return;
    }
    if (byte == '\\' && !apc_string_.empty() && apc_string_.back() == 0x1B) {
        apc_string_.pop_back(); // Remove ESC
        execute_kitty_graphics();
        state_ = ParserState::Ground;
        return;
    }
    if (!apc_string_.empty() && apc_string_.back() == 0x1B) {
        apc_string_.pop_back();
        execute_kitty_graphics();
        state_ = ParserState::Escape;
        handle_escape(byte);
        return;
    }

    apc_string_.push_back(static_cast<char>(byte));
}

void AnsiParser::execute_kitty_graphics() {
    if (apc_string_.empty() || apc_string_[0] != 'G') {
        return;
    }

    // Format: G<k>=<v>,<k>=<v>...;<payload>
    size_t semi_pos = apc_string_.find(';');
    std::string keys_str = (semi_pos != std::string::npos) ? apc_string_.substr(1, semi_pos - 1) : apc_string_.substr(1);
    std::string payload = (semi_pos != std::string::npos) ? apc_string_.substr(semi_pos + 1) : "";

    // Parse keys
    std::stringstream ss(keys_str);
    std::string token;
    while (std::getline(ss, token, ',')) {
        size_t eq = token.find('=');
        if (eq != std::string::npos) {
            std::string k = token.substr(0, eq);
            std::string v = token.substr(eq + 1);
            kitty_active_params_[k] = v;
        }
    }

    // Accumulate payload
    kitty_chunk_payload_ += payload;

    // Check if more chunks are expected
    auto it_m = kitty_active_params_.find("m");
    if (it_m != kitty_active_params_.end() && it_m->second == "1") {
        return; // Wait for m=0
    }

    auto get_str = [this](const std::string& key, const std::string& def) -> std::string {
        auto it = kitty_active_params_.find(key);
        return (it != kitty_active_params_.end()) ? it->second : def;
    };
    auto get_int = [&get_str](const std::string& key, int def) -> int {
        std::string s = get_str(key, "");
        if (s.empty()) return def;
        try { return std::stoi(s); } catch (...) { return def; }
    };

    std::string action = get_str("a", !kitty_chunk_payload_.empty() ? "T" : "t");
    int format = get_int("f", 100);
    std::string medium = get_str("t", "d");
    int src_w = get_int("s", 0);
    int src_h = get_int("v", 0);
    int cols = get_int("c", 0);
    int rows = get_int("r", 0);
    uint32_t image_id = static_cast<uint32_t>(get_int("i", 0));
    uint32_t placement_id = static_cast<uint32_t>(get_int("p", 0));
    int quiet = get_int("q", 0);
    int crop_x = get_int("x", 0);
    int crop_y = get_int("y", 0);
    int crop_w = get_int("w", 0);
    int crop_h = get_int("h", 0);
    bool dont_move_cursor = (get_str("C", "0") == "1");

    if (action == "d" || action == "D") {
        buffer_.delete_images(image_id);
        if (quiet == 0 && response_callback_) {
            response_callback_("\033_Gi=" + std::to_string(image_id) + ";OK\033\\");
        }
    } else if (action == "q" || action == "Q") {
        if (response_callback_) {
            response_callback_("\033_Gi=" + std::to_string(image_id) + ";OK\033\\");
        }
    } else if (action == "t" || action == "T") {
        std::vector<uint8_t> image_bytes;
        if (medium == "f") {
            std::string file_path;
            auto decoded_path = base64_decode(kitty_chunk_payload_);
            if (!decoded_path.empty()) {
                file_path = std::string(decoded_path.begin(), decoded_path.end());
            } else {
                file_path = kitty_chunk_payload_;
            }
            std::ifstream file(file_path, std::ios::binary);
            if (file.is_open()) {
                image_bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            }
        } else {
            image_bytes = base64_decode(kitty_chunk_payload_);
        }

        if (!image_bytes.empty()) {
            if (image_id == 0) {
                image_id = next_image_id_++;
            }
            auto img = create_image_from_memory(image_id, image_bytes.data(), image_bytes.size(), format, src_w, src_h);
            if (img) {
                buffer_.add_image(img);

                if (action == "T") {
                    if (cols <= 0 && rows <= 0) {
                        cols = std::clamp((img->pixel_width + 8) / 9, 1, buffer_.get_cols());
                        rows = std::clamp((img->pixel_height + 17) / 18, 1, buffer_.get_rows());
                    } else if (cols <= 0 && rows > 0) {
                        cols = std::max(1, (rows * 18 * img->pixel_width) / std::max(1, img->pixel_height * 9));
                    } else if (rows <= 0 && cols > 0) {
                        rows = std::max(1, (cols * 9 * img->pixel_height) / std::max(1, img->pixel_width * 18));
                    }

                    ImagePlacement placement;
                    placement.image_id = img->id;
                    placement.placement_id = placement_id;
                    placement.image = img;
                    placement.start_col = buffer_.get_cursor_col();
                    placement.cols = cols;
                    placement.rows = rows;
                    placement.src_x = crop_x;
                    placement.src_y = crop_y;
                    placement.src_w = crop_w;
                    placement.src_h = crop_h;

                    buffer_.place_image(placement);

                    if (!dont_move_cursor) {
                        for (int r = 0; r < rows; ++r) {
                            buffer_.newline();
                        }
                        buffer_.carriage_return();
                    }
                }

                if (quiet == 0 && response_callback_) {
                    response_callback_("\033_Gi=" + std::to_string(image_id) + ";OK\033\\");
                }
            } else if (quiet != 2 && response_callback_) {
                response_callback_("\033_Gi=" + std::to_string(image_id) + ";EINVAL\033\\");
            }
        }
    } else if (action == "p" || action == "P") {
        auto img = buffer_.get_image(image_id);
        if (img) {
            if (cols <= 0 && rows <= 0) {
                cols = std::clamp((img->pixel_width + 8) / 9, 1, buffer_.get_cols());
                rows = std::clamp((img->pixel_height + 17) / 18, 1, buffer_.get_rows());
            } else if (cols <= 0 && rows > 0) {
                cols = std::max(1, (rows * 18 * img->pixel_width) / std::max(1, img->pixel_height * 9));
            } else if (rows <= 0 && cols > 0) {
                rows = std::max(1, (cols * 9 * img->pixel_height) / std::max(1, img->pixel_width * 18));
            }

            ImagePlacement placement;
            placement.image_id = img->id;
            placement.placement_id = placement_id;
            placement.image = img;
            placement.start_col = buffer_.get_cursor_col();
            placement.cols = cols;
            placement.rows = rows;
            placement.src_x = crop_x;
            placement.src_y = crop_y;
            placement.src_w = crop_w;
            placement.src_h = crop_h;

            buffer_.place_image(placement);

            if (!dont_move_cursor) {
                for (int r = 0; r < rows; ++r) {
                    buffer_.newline();
                }
                buffer_.carriage_return();
            }

            if (quiet == 0 && response_callback_) {
                response_callback_("\033_Gi=" + std::to_string(image_id) + ";OK\033\\");
            }
        }
    }

    kitty_chunk_payload_.clear();
    kitty_active_params_.clear();
}

} // namespace evaterm
