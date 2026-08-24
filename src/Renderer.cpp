#include "Renderer.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <cmath>

namespace evaterm {

Renderer::Renderer(FontAtlas& font_atlas, const Config& config)
    : font_atlas_(font_atlas), config_(config) {}

void Renderer::init() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
}

void Renderer::draw_quad(float x, float y, float w, float h, const Color& color) {
    glDisable(GL_TEXTURE_2D);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

void Renderer::draw_char(float x, float y, char32_t codepoint, const Color& color) {
    if (codepoint == ' ' || codepoint == 0) return;

    const GlyphInfo& glyph = font_atlas_.get_glyph(codepoint);
    if (!glyph.loaded || glyph.width == 0 || glyph.height == 0) return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, font_atlas_.get_texture_id());
    glColor4ub(color.r, color.g, color.b, color.a);

    float gx = std::floor(x + glyph.bearing_x);
    float gy = std::floor(y + (font_atlas_.get_baseline() - glyph.bearing_y));
    float gw = static_cast<float>(glyph.width);
    float gh = static_cast<float>(glyph.height);

    glBegin(GL_QUADS);
    glTexCoord2f(glyph.u0, glyph.v0); glVertex2f(gx, gy);
    glTexCoord2f(glyph.u1, glyph.v0); glVertex2f(gx + gw, gy);
    glTexCoord2f(glyph.u1, glyph.v1); glVertex2f(gx + gw, gy + gh);
    glTexCoord2f(glyph.u0, glyph.v1); glVertex2f(gx, gy + gh);
    glEnd();
}

void Renderer::draw_text(float x, float y, const std::string& text, const Color& color, int max_width) {
    float cur_x = x;
    int cell_w = font_atlas_.get_cell_width();

    for (size_t i = 0; i < text.size(); ++i) {
        if (max_width > 0 && (cur_x + cell_w > x + max_width)) {
            // Truncate with ellipsis if space allows
            draw_char(cur_x, y, U'.', color);
            break;
        }

        uint8_t b = static_cast<uint8_t>(text[i]);
        char32_t cp = b;
        if ((b & 0x80) != 0) {
            // Simple multibyte skip or single byte fallback
            if ((b & 0xE0) == 0xC0 && i + 1 < text.size()) {
                cp = ((b & 0x1F) << 6) | (static_cast<uint8_t>(text[i + 1]) & 0x3F);
                i += 1;
            } else if ((b & 0xF0) == 0xE0 && i + 2 < text.size()) {
                cp = ((b & 0x0F) << 12) | ((static_cast<uint8_t>(text[i + 1]) & 0x3F) << 6) | (static_cast<uint8_t>(text[i + 2]) & 0x3F);
                i += 2;
            }
        }

        draw_char(cur_x, y, cp, color);
        cur_x += cell_w;
    }
}

void Renderer::render(const TabManager& tab_manager, const TabBar& tab_bar, int window_width, int window_height, bool window_focused) {
    // Setup 2D orthographic projection
    glViewport(0, 0, window_width, window_height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, window_width, window_height, 0.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Clear background
    const auto& theme = config_.theme;
    uint8_t bg_alpha = static_cast<uint8_t>(std::clamp(theme.background_opacity, 0.0f, 1.0f) * 255);
    glClearColor(theme.background.r / 255.0f, theme.background.g / 255.0f, theme.background.b / 255.0f, bg_alpha / 255.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Render Tab Bar if multiple tabs or configured to show
    bool show_tabs = config_.show_tab_bar_single_tab || tab_manager.get_tab_count() > 1;
    int tab_bar_h = show_tabs ? tab_bar.get_height() : 0;

    if (show_tabs) {
        render_tab_bar(tab_bar, window_width);
    }

    // Render Active Tab Terminal Content
    auto active_tab = tab_manager.get_active_tab();
    if (active_tab) {
        int grid_top = tab_bar_h + config_.padding_y;
        int grid_left = config_.padding_x;
        const auto& buffer = active_tab->get_terminal().get_buffer();

        render_terminal_grid(buffer, grid_top, grid_left, window_width, window_height, window_focused);
        render_cursor(buffer, grid_top, grid_left, window_focused);
        render_scroll_indicator(buffer, tab_bar_h, window_width, window_height);
    }
}

void Renderer::render_tab_bar(const TabBar& tab_bar, int window_width) {
    const auto& theme = config_.theme;
    float bar_h = static_cast<float>(tab_bar.get_height());

    // 1. Tab bar background strip
    draw_quad(0, 0, static_cast<float>(window_width), bar_h, theme.tab_bar_bg);

    // 2. Bottom separator line
    draw_quad(0, bar_h - 1, static_cast<float>(window_width), 1, theme.tab_bar_border);

    // 3. Render tabs
    const auto& layouts = tab_bar.get_layouts();
    for (size_t i = 0; i < layouts.size(); ++i) {
        const auto& item = layouts[i];
        bool is_hovered = (tab_bar.get_hovered_tab() == static_cast<int>(i));

        // Tab background
        Color tab_bg = item.is_active ? theme.active_tab_bg : theme.inactive_tab_bg;
        if (!item.is_active && is_hovered) {
            // Lighten inactive hovered tab
            tab_bg.r = std::min(255, tab_bg.r + 20);
            tab_bg.g = std::min(255, tab_bg.g + 20);
            tab_bg.b = std::min(255, tab_bg.b + 20);
        }
        draw_quad(item.bounds.x, item.bounds.y, item.bounds.w, item.bounds.h - (item.is_active ? 0 : 1), tab_bg);

        // Active tab top accent line
        if (item.is_active) {
            draw_quad(item.bounds.x, item.bounds.y, item.bounds.w, 2, theme.cursor);
        }

        // Tab title text
        Color tab_fg = item.is_active ? theme.active_tab_fg : theme.inactive_tab_fg;
        float text_x = item.bounds.x + 10.0f;
        float text_y = item.bounds.y + (bar_h - font_atlas_.get_cell_height()) / 2.0f;
        int max_title_w = static_cast<int>(item.bounds.w - 36.0f);
        draw_text(text_x, text_y, item.title, tab_fg, max_title_w);

        // Close button (x)
        bool close_hovered = (tab_bar.get_hovered_close() == static_cast<int>(i));
        Color close_col = close_hovered ? theme.cursor : (item.is_active ? theme.active_tab_fg : theme.inactive_tab_fg);
        float cx = item.close_button_bounds.x;
        float cy = item.close_button_bounds.y;
        
        if (close_hovered) {
            Color btn_bg = theme.background;
            btn_bg.a = 150;
            draw_quad(cx - 2, cy - 2, 18, 18, btn_bg);
        }
        draw_text(cx + 3, cy - 1, "x", close_col);
    }

    // 4. Render '+' Add Tab Button
    const auto& add_btn = tab_bar.get_add_button_bounds();
    bool add_hovered = tab_bar.get_hovered_add();
    Color add_bg = add_hovered ? theme.active_tab_bg : theme.inactive_tab_bg;
    Color add_fg = add_hovered ? theme.active_tab_fg : theme.inactive_tab_fg;
    draw_quad(add_btn.x, add_btn.y, add_btn.w, add_btn.h, add_bg);
    draw_text(add_btn.x + 7, add_btn.y + (add_btn.h - font_atlas_.get_cell_height()) / 2.0f, "+", add_fg);
}

void Renderer::render_terminal_grid(const ScreenBuffer& buffer, int grid_top, int grid_left, int window_width, int window_height, bool window_focused) {
    const auto& theme = config_.theme;
    int rows = buffer.get_rows();
    int cols = buffer.get_cols();
    int cell_w = font_atlas_.get_cell_width();
    int cell_h = font_atlas_.get_cell_height();

    // 1. Pass 1: Backgrounds & Selection highlights
    for (int r = 0; r < rows; ++r) {
        float py = static_cast<float>(grid_top + r * cell_h);
        if (py + cell_h > window_height) break;

        for (int c = 0; c < cols; ++c) {
            float px = static_cast<float>(grid_left + c * cell_w);
            if (px + cell_w > window_width) break;

            const Cell& cell = buffer.get_cell(r, c);
            bool is_selected = buffer.is_cell_selected(r, c);

            Color bg = cell.is_default_bg ? theme.background : cell.bg;
            Color fg = cell.is_default_fg ? theme.foreground : cell.fg;

            if (cell.attributes & ATTR_INVERSE) {
                std::swap(bg, fg);
            }

            if (is_selected) {
                bg = theme.selection_bg;
            }

            if (bg != theme.background || is_selected) {
                draw_quad(px, py, static_cast<float>(cell_w), static_cast<float>(cell_h), bg);
            }
        }
    }

    // 2. Pass 2: Foreground Glyphs and Text Attributes
    for (int r = 0; r < rows; ++r) {
        float py = static_cast<float>(grid_top + r * cell_h);
        if (py + cell_h > window_height) break;

        for (int c = 0; c < cols; ++c) {
            float px = static_cast<float>(grid_left + c * cell_w);
            if (px + cell_w > window_width) break;

            const Cell& cell = buffer.get_cell(r, c);
            if (cell.codepoint == ' ' || cell.codepoint == 0) continue;
            if (cell.attributes & ATTR_HIDDEN) continue;

            bool is_selected = buffer.is_cell_selected(r, c);
            Color fg = cell.is_default_fg ? theme.foreground : cell.fg;
            Color bg = cell.is_default_bg ? theme.background : cell.bg;

            if (cell.attributes & ATTR_INVERSE) {
                std::swap(fg, bg);
            }

            if (is_selected) {
                fg = theme.selection_fg;
            }

            if (cell.attributes & ATTR_DIM) {
                fg.r = static_cast<uint8_t>(fg.r * 0.6f);
                fg.g = static_cast<uint8_t>(fg.g * 0.6f);
                fg.b = static_cast<uint8_t>(fg.b * 0.6f);
            }

            draw_char(px, py, cell.codepoint, fg);

            // Underline
            if (cell.attributes & ATTR_UNDERLINE) {
                float line_y = py + font_atlas_.get_baseline() + 2.0f;
                draw_quad(px, line_y, static_cast<float>(cell_w), 1.0f, fg);
            }

            // Strikethrough
            if (cell.attributes & ATTR_STRIKETHROUGH) {
                float line_y = py + cell_h / 2.0f;
                draw_quad(px, line_y, static_cast<float>(cell_w), 1.0f, fg);
            }
        }
    }
}

void Renderer::render_cursor(const ScreenBuffer& buffer, int grid_top, int grid_left, bool window_focused) {
    if (!buffer.is_cursor_visible() || buffer.get_scroll_offset() > 0) return;

    int cursor_row = buffer.get_cursor_row();
    int cursor_col = buffer.get_cursor_col();
    int cell_w = font_atlas_.get_cell_width();
    int cell_h = font_atlas_.get_cell_height();

    float px = static_cast<float>(grid_left + cursor_col * cell_w);
    float py = static_cast<float>(grid_top + cursor_row * cell_h);

    const auto& theme = config_.theme;

    // Check cursor blink
    if (config_.cursor_blink && window_focused) {
        uint32_t ticks = SDL_GetTicks();
        if ((ticks / config_.cursor_blink_interval_ms) % 2 != 0) {
            return; // Cursor currently in blink hidden phase
        }
    }

    if (!window_focused) {
        // Hollow rectangle when unfocused
        draw_quad(px, py, static_cast<float>(cell_w), 1, theme.cursor);
        draw_quad(px, py + cell_h - 1, static_cast<float>(cell_w), 1, theme.cursor);
        draw_quad(px, py, 1, static_cast<float>(cell_h), theme.cursor);
        draw_quad(px + cell_w - 1, py, 1, static_cast<float>(cell_h), theme.cursor);
        return;
    }

    switch (config_.cursor_shape) {
        case CursorShape::Beam:
            draw_quad(px, py, 2.0f, static_cast<float>(cell_h), theme.cursor);
            break;
        case CursorShape::Underline:
            draw_quad(px, py + cell_h - 2.0f, static_cast<float>(cell_w), 2.0f, theme.cursor);
            break;
        case CursorShape::Block:
        default: {
            draw_quad(px, py, static_cast<float>(cell_w), static_cast<float>(cell_h), theme.cursor);
            // Draw character inverted under block cursor
            const Cell& cell = buffer.get_cell(cursor_row, cursor_col);
            if (cell.codepoint != ' ' && cell.codepoint != 0) {
                draw_char(px, py, cell.codepoint, theme.cursor_text);
            }
            break;
        }
    }
}

void Renderer::render_scroll_indicator(const ScreenBuffer& buffer, int grid_top, int window_width, int window_height) {
    int scroll_offset = buffer.get_scroll_offset();
    if (scroll_offset == 0) return;

    size_t scroll_total = buffer.get_scrollback_size();
    if (scroll_total == 0) return;

    float view_h = static_cast<float>(window_height - grid_top);
    float track_w = 4.0f;
    float track_x = static_cast<float>(window_width - track_w - 2);

    float thumb_h = std::max(20.0f, view_h * (static_cast<float>(buffer.get_rows()) / (scroll_total + buffer.get_rows())));
    float progress = 1.0f - (static_cast<float>(scroll_offset) / scroll_total);
    float thumb_y = grid_top + progress * (view_h - thumb_h);

    Color thumb_col = config_.theme.cursor;
    thumb_col.a = 180;
    draw_quad(track_x, thumb_y, track_w, thumb_h, thumb_col);
}

} // namespace evaterm
