#pragma once
#include <string>
#include <cstdint>

bool save_framebuffer_to_png(const std::string& filename, int width, int height);

std::string generate_screenshot_filename(const std::string& art_name, unsigned frame_number);
