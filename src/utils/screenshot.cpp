#include "screenshot.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "external/stb/stb_image_write.h"

#include <GL/gl.h>

#include <vector>
#include <cstring>
#include <cstdio>

bool save_framebuffer_to_png(const std::string& filename, int width, int height) {
    std::vector<unsigned char> pixels(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    
    std::vector<unsigned char> flipped(width * height * 4);
    for (int y = 0; y < height; y++) {
        std::memcpy(&flipped[y * width * 4], 
                    &pixels[(height - 1 - y) * width * 4], 
                    width * 4);
    }
    
    return stbi_write_png(filename.c_str(), width, height, 4, flipped.data(), width * 4) != 0;
}

std::string generate_screenshot_filename(const std::string& art_name, unsigned frame_number) {
    char filename[256];
    std::snprintf(filename, sizeof(filename), "%s_%08u.png", art_name.c_str(), frame_number);
    return std::string(filename);
}
