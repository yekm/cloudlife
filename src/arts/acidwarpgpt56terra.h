#pragma once

#include "art.hpp"

#include <string>

class AcidWarpGpt56Terra : public Art {
public:
    std::string about() const override
    {
        return "History and provenance\n"
               "This is Cloudlife's Gpt56Terra GPU interpretation of AcidWarp. Noah Spurrier created the "
               "original DOS eye-candy program; README.md and arχiv/acidwarp/acidwarp.c record copyright years "
               "1992 and 1993. The archived README credits Steven Wills with the Linux port. The modern shader's "
               "comments describe its relationship to that program but do not record a separate creation date or "
               "author for this variant. The original credits should not be read as authorship of the new "
               "shader.\n"
               "\n"
               "Algorithm\n"
               "A compute-shader invocation constructs a centered coordinate for each pixel, correcting the "
               "horizontal coordinate for image aspect ratio, then multiplies by pattern scale. The selected "
               "image function evaluates a scalar field from polar radius, atan-based angle, sine rings, and "
               "horizontal and vertical cosine waves. Several functions sum rings around multiple centers; those "
               "centers follow different sine and cosine trajectories as time advances. Others generate "
               "peacock-like interference, angular spokes, concentric bands, products of rings, repeated grids, "
               "or combinations with a vertically stretched coordinate system.\n"
               "\n"
               "Function numbers 28, 29, 33, and 34 produce diagonal trends mixed with deterministic cell "
               "hashes. These replace the spatially recursive rain patterns of classic AcidWarp with "
               "independently evaluable procedural fields. They retain a related visual family without running "
               "the original neighbor-dependent algorithm. The shader contains 41 field branches; the current "
               "image-function slider exposes numbers 0 through 39.\n"
               "\n"
               "Color and geometric movement are separate. Motion speed multiplies elapsed time used in the "
               "field equations and moving centers. Palette speed adds an elapsed-time phase to the field, whose "
               "fractional part becomes a colormap coordinate. Changing the palette swaps the shader's colormap "
               "function. Zero motion can therefore leave the geometry stationary while its colors continue to "
               "flow. Unlike the state-machine variants, this version continuously displays the selected "
               "function without image fade transitions or independent channel rolls. Scale changes the density "
               "and apparent size of features; the controls expose function selection, scale, motion speed, and "
               "palette speed.\n"
               "\n"
               "References\n"
               "https://noah.org/acidwarp/\n"
               "https://en.wikipedia.org/wiki/Color_cycling\n"
               "https://en.wikipedia.org/wiki/Procedural_texture";
    }

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
