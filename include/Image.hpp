#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <string_view>
#include <GL/gl.h>

namespace evaterm {

struct ImageData {
    uint32_t id = 0;
    int pixel_width = 0;
    int pixel_height = 0;
    std::vector<uint8_t> rgba_data; // 32-bit RGBA pixels
    GLuint texture_id = 0;
    bool texture_uploaded = false;

    ~ImageData();
    void upload_gl_texture();
    void delete_gl_texture();
};

struct ImagePlacement {
    uint32_t image_id = 0;
    uint32_t placement_id = 0;
    std::shared_ptr<ImageData> image;

    int start_col = 0;
    int cols = 0;
    int rows = 0;

    int src_x = 0;
    int src_y = 0;
    int src_w = 0;
    int src_h = 0;

    int64_t absolute_line = 0;
    bool in_alt_buffer = false;
};

// Base64 decoding helper
std::vector<uint8_t> base64_decode(const std::string_view& in);

// Image loading helper from memory buffer (PNG, JPEG, GIF, BMP, raw RGBA/RGB)
std::shared_ptr<ImageData> create_image_from_memory(
    uint32_t id,
    const uint8_t* data,
    size_t size,
    int format, // 100 = PNG/auto, 32 = raw RGBA, 24 = raw RGB
    int src_width = 0,
    int src_height = 0
);

} // namespace evaterm
