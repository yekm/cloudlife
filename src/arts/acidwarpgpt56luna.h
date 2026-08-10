#pragma once

#include "art.hpp"

#include <string>

class AcidWarpGpt56Luna : public Art {
public:
    AcidWarpGpt56Luna();

private:
    bool render(uint32_t* pixels) override;
    bool render_gui() override;
    void resize(int width, int height) override;

    void init_shader();
    void update_uniform_callback();
    void choose_pattern();

    enum State { IMAGE, FADE_IN, ROTATE, FADE_OUT } state = IMAGE;

    int frame = 0;
    int frames_each_state = 60 * 10;
    int image_function = 41; // 0..40, 41 selects a new function per image.
    int current_function = 0;
    float centers[8] = {};
    float pattern_scale = 1.0f;
    float motion_speed = 0.0f;
    float palette_speed = 1.0f;
    float palette_phase = 0.0f;
    float fade = 1.0f;
    bool fade_to_white = false;
    std::string current_colormap_name;

    static const char* compute_shader_base;
};
