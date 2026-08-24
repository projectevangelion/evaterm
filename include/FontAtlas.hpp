#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include <SDL2/SDL_opengl.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace evaterm {

struct GlyphInfo {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    int width = 0;
    int height = 0;
    int bearing_x = 0;
    int bearing_y = 0;
    int advance_x = 0;
    bool loaded = false;
};

class FontAtlas {
public:
    FontAtlas();
    ~FontAtlas();

    bool init(const std::string& font_family, int pt_size);
    void cleanup();

    const GlyphInfo& get_glyph(char32_t codepoint);

    int get_cell_width() const { return cell_width_; }
    int get_cell_height() const { return cell_height_; }
    int get_baseline() const { return baseline_; }
    GLuint get_texture_id() const { return texture_id_; }

    void set_font_size(int pt_size);
    int get_font_size() const { return pt_size_; }

private:
    FT_Library ft_library_ = nullptr;
    FT_Face ft_face_ = nullptr;

    std::string font_family_;
    int pt_size_ = 13;

    int cell_width_ = 9;
    int cell_height_ = 18;
    int baseline_ = 14;

    GLuint texture_id_ = 0;
    static constexpr int ATLAS_WIDTH = 2048;
    static constexpr int ATLAS_HEIGHT = 2048;

    std::vector<uint32_t> atlas_pixels_; // RGBA32 format (0xAABBGGRR / white with alpha)
    int atlas_cursor_x_ = 2;
    int atlas_cursor_y_ = 2;
    int atlas_row_height_ = 0;

    std::unordered_map<char32_t, GlyphInfo> glyph_cache_;

    bool load_face();
    std::string resolve_font_path(const std::string& family);
    GlyphInfo render_and_cache_glyph(char32_t codepoint);
};

} // namespace evaterm
