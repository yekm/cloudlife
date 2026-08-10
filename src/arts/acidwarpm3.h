#pragma once
#include "art.hpp"

#include <string>

class AcidWarpM3 : public Art {
public:
    AcidWarpM3();

private:
    bool render(uint32_t *p) override;
    bool render_gui() override;
    void resize(int _w, int _h) override;

    void init_shader();
    void update_uniform_callback();
    void pick_pattern();

    enum State { IMAGE, FADE_IN, ROTATE, FADE_OUT } state = IMAGE;

    int frame = 0;
    int frames_per_state = 60 * 10;
    int func_mode = 41;          // 0..40 fixed, 41 = random
    int current_func = 0;
    float centers[8] = {};       // 4 vec2 random peacock centers
    float roll[3] = {};          // per-channel R/G/B palette roll phase
    int roll_dir[3] = {1, 1, 1};
    float fade = 1.0f;
    bool fade_to_white = false;
    float motion_speed = 0.0f;
    float pattern_scale = 1.0f;
    float palette_speed = 1.0f;
    float sparkle = 0.0f;
    std::string current_colormap_name;

    static const char *compute_shader_base;
};
