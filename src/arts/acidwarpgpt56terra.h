#pragma once

#include "art.hpp"

#include <string>

class AcidWarpGpt56Terra : public Art {
public:
    AcidWarpGpt56Terra();

private:
    bool render(uint32_t* pixels) override;
    bool render_gui() override;
    void resize(int width, int height) override;

    void init_shader();
    void update_uniform_callback();

    int image_function = 0;
    float pattern_scale = 1.0f;
    float motion_speed = 0.18f;
    float palette_speed = 0.08f;
    std::string current_colormap_name;

    static const char* compute_shader_base;
};
