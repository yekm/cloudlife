#pragma once
#include "art.hpp"

#include <string>

class AcidWarpM3 : public Art {
public:
    std::string about() const override
    {
        return "History and provenance\n"
               "The source identifies this as AcidWarp-minimaxm3, a GPU rework of classic AcidWarp. The original "
               "program was written by Noah Spurrier, with copyright years 1992 and 1993 recorded in readme.md "
               "and the archived source. Steven Wills is credited for its Linux port in arχiv/acidwarp/README. "
               "The available variant comments explain the rewrite but do not record a separate author or "
               "creation date; the original author's credit establishes its lineage rather than authorship of "
               "this shader.\n"
               "\n"
               "Algorithm\n"
               "The compute shader evaluates 41 image functions independently per pixel. It translates the "
               "original integer lookup-table conventions, including 256 angle units and a sine amplitude near "
               "511, into floating-point trigonometry. Centered distance, polar angle, Cartesian waves, "
               "off-center ring sums, nested trigonometric functions, and quantized XOR combinations form the "
               "scalar image. Random centers diversify the multi-center patterns. The field is reduced to a "
               "repeating palette coordinate with fract(c / 255).\n"
               "\n"
               "Classic rain modes 28, 29, 33, and 34 refer to earlier pixels in a scanline. This version "
               "replaces those recurrences with smoothstep-interpolated value noise at multiple scales. The "
               "approximation allows parallel evaluation and gives smoothed, diffuse patterns rather than "
               "claiming exact agreement with the original images.\n"
               "\n"
               "Red, green, and blue have separate palette phases. The shader samples the selected colormap at "
               "each phase and assembles the corresponding channels; when phases coincide it can use a single "
               "sample. Each channel rolls once per frame and can randomly reverse direction. Sparkle brightens "
               "every fourth quantized palette entry toward white. A CPU state machine selects a pattern, fades "
               "in, holds palette rotation, and fades out toward black or white. Fades take roughly 63 frames to "
               "reach their endpoints; the state duration also controls pauses. Function 41 selects a new random "
               "function for each image. The other controls set pattern density, optional geometric motion, "
               "palette speed, sparkle strength, state duration, and fade color.\n"
               "\n"
               "References\n"
               "https://noah.org/acidwarp/\n"
               "https://en.wikipedia.org/wiki/Color_cycling\n"
               "https://en.wikipedia.org/wiki/Value_noise";
    }

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
