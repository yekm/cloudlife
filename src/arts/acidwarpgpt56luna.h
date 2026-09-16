#pragma once

#include "art.hpp"

#include <string>

class AcidWarpGpt56Luna : public Art {
public:
    std::string about() const override
    {
        return "History and provenance\n"
               "This is Cloudlife's Gpt56Luna compute-shader interpretation of AcidWarp. The inherited program "
               "was created by Noah Spurrier: readme.md and the archived AcidWarp source carry his 1992 and 1993 "
               "copyright notice. The archive's README identifies Steven Wills as the author of the Linux port "
               "of the DOS program. These credits describe the original lineage; the available comments do not "
               "establish the author or creation date of this particular GPU variant.\n"
               "\n"
               "Algorithm\n"
               "Each pixel is evaluated independently. Pixel coordinates are centered and divided by half the "
               "shorter image dimension, preserving circular geometry across different aspect ratios. The "
               "pattern scale multiplies those coordinates. A selected scalar field combines distance from the "
               "center, atan-based polar angle, horizontal and vertical trigonometric waves, and rings around "
               "several randomly chosen centers. Other functions combine fields by multiplication, make angular "
               "sectors and repeated grids, or mix a vertically stretched field with the original. The shader "
               "implements function numbers 0 through 40 as interpretations of the classic pattern families.\n"
               "\n"
               "The original recursive rain functions depend on previously calculated scanline pixels. Functions "
               "28, 29, 33, and 34 instead use a rotated, quantized hash field to create diagonal streaks that "
               "can be calculated in parallel. This is an approximation, not a reproduction of the original "
               "recurrence.\n"
               "\n"
               "The fractional part of the field plus a palette phase selects the active colormap. All three "
               "color channels share this phase. Palette speed advances it once per rendered frame, so colors "
               "can flow even with geometric motion set to zero. A CPU state machine selects an image, fades it "
               "in, holds the rotating palette, and fades out toward black or white. Fades reach their endpoint "
               "after about 63 frames; frames each state also sets the hold time. Controls choose the function, "
               "scale, motion, palette speed, state duration, and fade color.\n"
               "\n"
               "References\n"
               "https://noah.org/acidwarp/\n"
               "https://en.wikipedia.org/wiki/Color_cycling\n"
               "https://en.wikipedia.org/wiki/Procedural_texture";
    }

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
