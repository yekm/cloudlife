#pragma once

#include "art.hpp"

#include <string>

class AcidWarpQwen38Max : public Art {
public:
    std::string about() const override
    {
        return "History and provenance\n"
               "The source calls this AcidWarp-qwen38-max, a GPU rework of classic AcidWarp. Noah Spurrier wrote "
               "the original DOS program; its 1992 and 1993 copyright appears in readme.md and "
               "arχiv/acidwarp/acidwarp.c. The archived README names Steven Wills as the Linux port's author. "
               "The shader's comments document its relationship to the original implementation but provide no "
               "separate author or creation date for this modern variant.\n"
               "\n"
               "Algorithm\n"
               "Every compute-shader invocation computes one pixel's scalar pattern. Coordinates relative to the "
               "image center provide radius and polar angle, while normalized image coordinates provide "
               "Cartesian wave arguments. Floating-point sine and cosine replace the original lookup-table "
               "arithmetic, retaining its 256-unit angle convention and approximately 511-unit sine amplitude. "
               "The 41 branches combine angular ramps, radial waves, rings around random centers, wave "
               "interference, nested trigonometry, integer XOR on quantized field values, and mixtures with a "
               "vertically stretched coordinate system.\n"
               "\n"
               "The original rain cases 28, 29, 33, and 34 used a recurrence involving left and upper neighbors. "
               "Here the coordinate axes are rotated diagonally, then hashes produce a mixture of "
               "continuous-looking streaks and finer grain. This deliberately approximate field has no "
               "dependency between pixels, so the GPU can evaluate it in parallel.\n"
               "\n"
               "A wrapped scalar field drives the selected colormap. Independent red, green, and blue phase "
               "offsets emulate separate rolls of the original VGA palette channels. Each phase advances per "
               "rendered frame, with a small random chance of reversing its direction. The shader assembles "
               "channels from up to three colormap samples. On the CPU, IMAGE, FADE_IN, ROTATE, and FADE_OUT "
               "states choose new patterns and mix the image toward black or white; fades reach the endpoint in "
               "about 63 frames, while frames each state controls additional waiting. The initial mode "
               "automatically chooses patterns; the slider selects fixed functions 0 through 40. Scale changes "
               "density, motion animates the pattern equations, roll speed animates colors, and the remaining "
               "controls set state duration and fade color.\n"
               "\n"
               "References\n"
               "https://noah.org/acidwarp/\n"
               "https://en.wikipedia.org/wiki/Color_cycling\n"
               "https://en.wikipedia.org/wiki/Procedural_texture";
    }

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
