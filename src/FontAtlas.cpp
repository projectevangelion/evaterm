#include "FontAtlas.hpp"
#include <fontconfig/fontconfig.h>
#include <iostream>
#include <cstring>
#include <cmath>

namespace evaterm {

FontAtlas::FontAtlas() {
    if (FT_Init_FreeType(&ft_library_)) {
        std::cerr << "Error: Failed to initialize FreeType library\n";
    }
}

FontAtlas::~FontAtlas() {
    cleanup();
    if (ft_library_) {
        FT_Done_FreeType(ft_library_);
        ft_library_ = nullptr;
    }
}

void FontAtlas::cleanup() {
    if (ft_face_) {
        FT_Done_Face(ft_face_);
        ft_face_ = nullptr;
    }
    if (texture_id_ != 0) {
        glDeleteTextures(1, &texture_id_);
        texture_id_ = 0;
    }
    glyph_cache_.clear();
    atlas_pixels_.clear();
    atlas_cursor_x_ = 2;
    atlas_cursor_y_ = 2;
    atlas_row_height_ = 0;
}

std::string FontAtlas::resolve_font_path(const std::string& family) {
    std::string font_path = "";
    FcConfig* config = FcInitLoadConfigAndFonts();
    if (!config) return font_path;

    auto try_match = [&](const std::string& fam) -> std::string {
        FcPattern* pattern = FcPatternCreate();
        if (!fam.empty()) {
            FcPatternAddString(pattern, FC_FAMILY, (const FcChar8*)fam.c_str());
        }
        FcPatternAddBool(pattern, FC_SCALABLE, FcTrue);

        FcConfigSubstitute(config, pattern, FcMatchPattern);
        FcDefaultSubstitute(pattern);

        FcResult result;
        FcPattern* match = FcFontMatch(config, pattern, &result);
        std::string found_file;
        if (match) {
            FcChar8* file = nullptr;
            if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch) {
                found_file = (const char*)file;
            }
            FcPatternDestroy(match);
        }
        FcPatternDestroy(pattern);
        return found_file;
    };

    std::string target = family.empty() ? "monospace" : family;
    font_path = try_match(target);

    if (font_path.empty()) {
        font_path = try_match("monospace");
    }

    FcConfigDestroy(config);
    return font_path;
}

bool FontAtlas::init(const std::string& font_family, int pt_size) {
    font_family_ = font_family;
    pt_size_ = pt_size > 0 ? pt_size : 13;

    cleanup();

    if (!load_face()) {
        return false;
    }

    atlas_pixels_.assign(ATLAS_WIDTH * ATLAS_HEIGHT, 0);

    // Generate OpenGL 32-bit RGBA texture
    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ATLAS_WIDTH, ATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, atlas_pixels_.data());

    // Pre-cache standard ASCII printable characters
    for (char32_t cp = 32; cp <= 126; ++cp) {
        render_and_cache_glyph(cp);
    }

    return true;
}

bool FontAtlas::load_face() {
    if (!ft_library_) return false;

    std::string font_path = resolve_font_path(font_family_);
    if (font_path.empty()) {
        std::cerr << "Error: Could not find any suitable font.\n";
        return false;
    }

    std::cout << "[EvaTerm] Loading font: " << font_path << " (size: " << pt_size_ << "pt)\n";

    if (FT_New_Face(ft_library_, font_path.c_str(), 0, &ft_face_)) {
        std::cerr << "Error: Failed to load font face from " << font_path << "\n";
        return false;
    }

    // Set pixel size (1 pt ≈ 1.333 px at standard 96 DPI)
    int pixel_size = static_cast<int>(std::round(pt_size_ * 1.333f));
    FT_Set_Pixel_Sizes(ft_face_, 0, pixel_size);

    // Compute monospace cell dimensions accurately
    if (FT_Load_Char(ft_face_, 'M', FT_LOAD_RENDER) == 0) {
        cell_width_ = ft_face_->glyph->advance.x >> 6;
    } else {
        cell_width_ = pixel_size / 2 + 1;
    }

    int ascender = ft_face_->size->metrics.ascender >> 6;
    int descender = ft_face_->size->metrics.descender >> 6;
    cell_height_ = (ascender - descender);
    baseline_ = ascender;

    if (cell_height_ <= 0) {
        cell_height_ = pixel_size + 4;
        baseline_ = pixel_size;
    }
    if (cell_width_ <= 0) {
        cell_width_ = pixel_size / 2 + 1;
    }

    return true;
}

void FontAtlas::set_font_size(int pt_size) {
    if (pt_size < 6) pt_size = 6;
    if (pt_size > 48) pt_size = 48;
    if (pt_size == pt_size_) return;

    init(font_family_, pt_size);
}

const GlyphInfo& FontAtlas::get_glyph(char32_t codepoint) {
    auto it = glyph_cache_.find(codepoint);
    if (it != glyph_cache_.end()) {
        return it->second;
    }

    GlyphInfo info = render_and_cache_glyph(codepoint);
    glyph_cache_[codepoint] = info;
    return glyph_cache_[codepoint];
}

GlyphInfo FontAtlas::render_and_cache_glyph(char32_t codepoint) {
    GlyphInfo info{};
    info.loaded = true;

    if (!ft_face_) return info;

    FT_UInt glyph_index = FT_Get_Char_Index(ft_face_, codepoint);
    if (glyph_index == 0 && codepoint != ' ') {
        return info;
    }

    if (FT_Load_Glyph(ft_face_, glyph_index, FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL)) {
        return info;
    }

    FT_GlyphSlot slot = ft_face_->glyph;
    FT_Bitmap& bmp = slot->bitmap;

    info.width = bmp.width;
    info.height = bmp.rows;
    info.bearing_x = slot->bitmap_left;
    info.bearing_y = slot->bitmap_top;
    info.advance_x = slot->advance.x >> 6;

    if (bmp.width == 0 || bmp.rows == 0) {
        return info;
    }

    // Wrap to next row in atlas if width exceeds
    if (atlas_cursor_x_ + bmp.width + 2 >= ATLAS_WIDTH) {
        atlas_cursor_x_ = 2;
        atlas_cursor_y_ += atlas_row_height_ + 2;
        atlas_row_height_ = 0;
    }

    if (atlas_cursor_y_ + bmp.rows + 2 >= ATLAS_HEIGHT) {
        return info;
    }

    // Build contiguous 32-bit RGBA glyph image (White color with font Alpha)
    std::vector<uint32_t> glyph_rgba(bmp.width * bmp.rows);
    for (unsigned int r = 0; r < bmp.rows; ++r) {
        for (unsigned int c = 0; c < bmp.width; ++c) {
            uint8_t alpha = bmp.buffer[r * bmp.pitch + c];
            uint32_t pixel = (static_cast<uint32_t>(alpha) << 24) | 0x00FFFFFF; // RGBA: (255, 255, 255, alpha)
            glyph_rgba[r * bmp.width + c] = pixel;

            int atlas_idx = (atlas_cursor_y_ + r) * ATLAS_WIDTH + (atlas_cursor_x_ + c);
            atlas_pixels_[atlas_idx] = pixel;
        }
    }

    // Upload only this sub-rectangle to GPU texture
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, atlas_cursor_x_, atlas_cursor_y_, bmp.width, bmp.rows,
                    GL_RGBA, GL_UNSIGNED_BYTE, glyph_rgba.data());

    info.u0 = static_cast<float>(atlas_cursor_x_) / ATLAS_WIDTH;
    info.v0 = static_cast<float>(atlas_cursor_y_) / ATLAS_HEIGHT;
    info.u1 = static_cast<float>(atlas_cursor_x_ + bmp.width) / ATLAS_WIDTH;
    info.v1 = static_cast<float>(atlas_cursor_y_ + bmp.rows) / ATLAS_HEIGHT;

    atlas_cursor_x_ += bmp.width + 2;
    if (static_cast<int>(bmp.rows) > atlas_row_height_) {
        atlas_row_height_ = bmp.rows;
    }

    glyph_cache_[codepoint] = info;
    return info;
}

} // namespace evaterm
