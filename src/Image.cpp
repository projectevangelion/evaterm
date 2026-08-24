#include "Image.hpp"
#include <cstring>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#include "stb_image.h"

namespace evaterm {

ImageData::~ImageData() {
    delete_gl_texture();
}

void ImageData::upload_gl_texture() {
    if (texture_uploaded || rgba_data.empty() || pixel_width <= 0 || pixel_height <= 0) {
        return;
    }

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        pixel_width,
        pixel_height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        rgba_data.data()
    );

    glBindTexture(GL_TEXTURE_2D, 0);
    texture_uploaded = true;
}

void ImageData::delete_gl_texture() {
    if (texture_uploaded && texture_id != 0) {
        glDeleteTextures(1, &texture_id);
        texture_id = 0;
        texture_uploaded = false;
    }
}

// Fast Base64 decoding table
static const uint8_t b64_table[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64,  0, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

std::vector<uint8_t> base64_decode(const std::string_view& in) {
    std::vector<uint8_t> out;
    if (in.empty()) return out;

    out.reserve((in.size() * 3) / 4);

    uint32_t val = 0;
    int valb = -8;
    for (uint8_t c : in) {
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') continue;
        if (c == '=') break;

        uint8_t b = b64_table[c];
        if (b >= 64) continue; // Skip invalid chars

        val = (val << 6) | b;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<uint8_t>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::shared_ptr<ImageData> create_image_from_memory(
    uint32_t id,
    const uint8_t* data,
    size_t size,
    int format,
    int src_width,
    int src_height
) {
    if (!data || size == 0) return nullptr;

    auto img = std::make_shared<ImageData>();
    img->id = id;

    if (format == 32) { // Raw 32-bit RGBA
        if (src_width <= 0 || src_height <= 0) return nullptr;
        img->pixel_width = src_width;
        img->pixel_height = src_height;
        img->rgba_data.assign(data, data + std::min(size, static_cast<size_t>(src_width * src_height * 4)));
        return img;
    } else if (format == 24) { // Raw 24-bit RGB -> convert to RGBA
        if (src_width <= 0 || src_height <= 0) return nullptr;
        img->pixel_width = src_width;
        img->pixel_height = src_height;
        img->rgba_data.resize(src_width * src_height * 4);

        size_t total_pixels = static_cast<size_t>(src_width * src_height);
        for (size_t i = 0; i < total_pixels && (i * 3 + 2) < size; ++i) {
            img->rgba_data[i * 4 + 0] = data[i * 3 + 0];
            img->rgba_data[i * 4 + 1] = data[i * 3 + 1];
            img->rgba_data[i * 4 + 2] = data[i * 3 + 2];
            img->rgba_data[i * 4 + 3] = 255;
        }
        return img;
    }

    // Default / PNG / JPEG / BMP / GIF (format == 100 or auto)
    int w = 0, h = 0, channels = 0;
    stbi_uc* decoded = stbi_load_from_memory(data, static_cast<int>(size), &w, &h, &channels, 4);
    if (!decoded) {
        std::cerr << "[EvaTerm] Failed to decode image: " << stbi_failure_reason() << "\n";
        return nullptr;
    }

    img->pixel_width = w;
    img->pixel_height = h;
    img->rgba_data.assign(decoded, decoded + (w * h * 4));
    stbi_image_free(decoded);

    return img;
}

} // namespace evaterm
