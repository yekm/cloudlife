#pragma once

#include "art.hpp"

#include <string>

class AcidWarpQwen38Max : public Art {
public:
    AcidWarpQwen38Max();

private:
    bool render(uint32_t *p) override;
    bool render_gui() override;
    void resize(int _w, int _h) override;

    void init_shader();
    void update_uniform_callback();
    void pick_pattern();

    enum { IMAGE, FADE_IN, ROTATE, FADE_OUT } acid_state = IMAGE;

    int frame = 0, frame_max = 60 * 10;
    int func_mode = 41;      // 0..40 fixed image function, above that: auto random
    int current_func = 0;
    float centers[8] = {};   // 4 random pattern centers (vec2), normalized units
    float roll[3] = {};      // per channel R,G,B palette roll phase, 0..1
    int roll_dir[3] = {1, 1, 1};
    float fade = 1.0f;
    bool fade_to_white = false;

    float pattern_scale = 1.0f;
    float motion_speed = 0.0f;
    float roll_speed = 1.0f; // palette entries per frame

    std::string current_colormap_name;

    static const char *compute_shader_base;
};
