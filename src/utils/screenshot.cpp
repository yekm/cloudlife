#include "screenshot.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <cstring>
#include <cstdio>
#include <limits>

bool save_framebuffer_to_png(const std::string& filename, int width, int height) {
    if (width <= 0 || height <= 0)
        return false;

    const auto height_size = static_cast<std::size_t>(height);
    const auto width_size = static_cast<std::size_t>(width);
    if (width_size > std::numeric_limits<std::size_t>::max() / height_size)
        return false;

    const auto pixel_count = width_size * height_size;
    if (pixel_count > std::numeric_limits<std::size_t>::max() / 4)
        return false;

    const auto row_size = width_size * 4;
    if (row_size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;

    std::vector<unsigned char> pixels(pixel_count * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    
    std::vector<unsigned char> flipped(width * height * 4);
    for (int y = 0; y < height; y++) {
        std::memcpy(&flipped[y * width * 4], 
                    &pixels[(height - 1 - y) * width * 4], 
                    row_size);
    }
    
    return stbi_write_png(filename.c_str(), width, height, 4, flipped.data(),
        static_cast<int>(row_size)) != 0;
}

std::string generate_screenshot_filename(const std::string& art_name, unsigned frame_number) {
    char filename[256];
    std::snprintf(filename, sizeof(filename), "%s_%08u.png", art_name.c_str(), frame_number);
    return std::string(filename);
}
