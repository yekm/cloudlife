/* AcidWarp-minimaxm3 - GPU rework of the classic AcidWarp hack
 * (Noah Spurrier, https://noah.org/acidwarp/).
 *
 * The original port (acidwarp.cpp) generates an indexed 256 color image on
 * the CPU every frame, then animates a 256x3 VGA palette (roll + fade state
 * machine) and converts every pixel through the palette on the CPU.
 *
 * This version does everything in a compute shader:
 *   - all 41 image functions of generate_image() are evaluated per pixel on
 *     the GPU, converted from the integer LUT math (lut.c: ANGLE_UNIT=256,
 *     sine amplitude +-511) to floating point;
 *   - the VGA palette is replaced by colormap-shaders colormaps appended to
 *     the shader source (see easel->pal);
 *   - the independent R/G/B palette roll of roll_rgb_palArray() is emulated
 *     as per-channel colormap phase offsets - the colormap is sampled three
 *     times with different offsets;
 *   - the IMAGE/FADE_IN/ROTATE/FADE_OUT state machine survives as an O(1)
 *     CPU side driver of a single vec4 fade uniform;
 *   - the optional lightning/sparkle palette brightness boost is implemented
 *     by tinting every 4th quantized palette entry toward white.
 *
 * CPU cost per frame is constant: state machine, palette roll phases and a
 * handful of uniforms. No per pixel work, no palette arrays, no upload.
 *
 * Cases 28/29/33/34 of the original are recursive (each pixel depends on its
 * left and upper neighbours) and cannot be evaluated in parallel; they are
 * approximated with multi-scale value noise that captures the smoothed
 * diffusion look of the original.
 */

#include "acidwarpm3.h"

#include "easelcompute.h"
#include "imgui.h"
#include "imgui_elements.h"
#include "random.h"

#include <algorithm>
#include <cmath>

const char *AcidWarpM3::compute_shader_base = R"(
    #version 430 core

    layout(local_size_x = 16, local_size_y = 16) in;

    layout(rgba8, binding = 0) uniform writeonly image2D output_image;

    uniform vec2 u_resolution;
    uniform float u_time;
    uniform int u_func;            // image function 0..41
    uniform float u_scale;         // pattern density
    uniform float u_motion;        // 0 = static like the original
    uniform vec4 u_centers[2];     // 4 random pattern centers (vec2)
    uniform vec3 u_roll;           // per channel colormap phase
    uniform vec4 u_fade;           // xyz = fade color, w = fade amount
    uniform float u_sparkle;       // 0..1 brightness boost on every 4th entry

    // Colormap function, injected from colormap-shaders
    vec4 colormap(float x);

    const float AU = 256.0;        // original ANGLE_UNIT
    const float TA = 511.0;        // original TRIG_UNIT sine amplitude
    const float QUART = 64.0;      // AU / 4
    const float OFF = 20.0;        // original fixed 20px peacock centers
    const float PI2 = 6.283185307179586;

    // AcidWarp LUT match: input in [0, 256], output in [-511, 511]
    float aw_sin(float a) { return TA * sin(a * PI2 / AU); }
    float aw_cos(float a) { return aw_sin(a + QUART); }

    // LUT-like angle in [0, 256]
    float angle_of(vec2 v) {
        return mod(atan(v.y, v.x) / PI2, 1.0) * AU;
    }

    float hash21(vec2 p) {
        return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
    }

    // Integer XOR of the original on quantized color values
    float fxor(float a, float b) {
        int ia = int(mod(a, 256.0));
        int ib = int(mod(b, 256.0));
        return float(ia ^ ib);
    }

    // Smooth value noise approximating the recursive diffusion of the original
    // cases 28/29/33/34. Multi-scale smoothstep-interpolated hash.
    float rain(vec2 q, float seed) {
        float h = 0.0;
        float amp = 0.55;
        vec2 p = q * 6.0;
        for (int i = 0; i < 3; ++i) {
            vec2 cell = floor(p);
            vec2 f = fract(p);
            f = f * f * (3.0 - 2.0 * f);
            float h00 = hash21(cell + vec2(seed, 0.0));
            float h10 = hash21(cell + vec2(1.0 + seed, 0.0));
            float h01 = hash21(cell + vec2(seed, 1.0));
            float h11 = hash21(cell + vec2(1.0 + seed, 1.0));
            float n = mix(mix(h00, h10, f.x), mix(h01, h11, f.x), f.y);
            h += n * amp;
            p *= 2.05;
            amp *= 0.5;
            seed += 7.31;
        }
        return h;
    }

    float field(vec2 pix, vec2 center, int mode, float ph,
                vec2 c0, vec2 c1, vec2 c2, vec2 c3, vec2 res) {
        vec2 q = pix - center;
        float d = length(q);
        float ang = angle_of(q);
        float ax2 = pix.x * AU / res.x * 2.0;
        float ay2 = pix.y * AU / res.y * 2.0;
        float wx2 = aw_cos(ax2 + ph);
        float wy2 = aw_cos(ay2 + ph);
        float wx7  = aw_cos(pix.x * AU / res.x * 7.0  + ph);
        float wy7  = aw_cos(pix.y * AU / res.y * 7.0  + ph);
        float wx11 = aw_cos(pix.x * AU / res.x * 11.0 + ph);
        float wy11 = aw_cos(pix.y * AU / res.y * 11.0 + ph);
        float wx17 = aw_cos(pix.x * AU / res.x * 17.0 + ph);
        float wy17 = aw_cos(pix.y * AU / res.y * 17.0 + ph);

        if (mode == 0) {
            return ang + aw_sin(d * 10.0 + ph) / 64.0 + wx2 / 32.0 + wy2 / 32.0;
        } else if (mode == 1) {
            return ang + aw_sin(d * 10.0 + ph) / 16.0 + wx2 / 8.0 + wy2 / 8.0;
        } else if (mode == 2) {
            return (aw_sin(length(q + c0) *  4.0 + ph)
                  + aw_sin(length(q + c1) *  8.0 + ph)
                  + aw_sin(length(q + c2) * 16.0 + ph)
                  + aw_sin(length(q + c3) * 32.0 + ph)) / 32.0;
        } else if (mode == 3) {
            return ang*2.0 + (aw_sin(length(q + vec2( OFF, 0.0)) * 10.0 + ph)
                            + aw_sin(length(q + vec2(-OFF, 0.0)) * 10.0 + ph)) / 32.0;
        } else if (mode == 4) {
            return aw_sin(d + ph) / 16.0;
        } else if (mode == 5) {
            return wx2 / 8.0 + wy2 / 8.0 + ang + aw_sin(d + ph) / 32.0;
        } else if (mode == 6) {
            return (aw_sin(length(q + vec2(0.0, -OFF)) *  4.0 + ph)
                  + aw_sin(length(q + vec2( OFF,  OFF)) *  4.0 + ph)
                  + aw_sin(length(q + vec2(-OFF,  OFF)) *  4.0 + ph)) / 32.0;
        } else if (mode == 7) {
            return ang + (aw_sin(length(q + vec2(0.0, -OFF)) *  8.0 + ph)
                        + aw_sin(length(q + vec2( OFF,  OFF)) *  8.0 + ph)
                        + aw_sin(length(q + vec2(-OFF,  OFF)) *  8.0 + ph)) / 32.0;
        } else if (mode == 8) {
            return (aw_sin(length(q + vec2(0.0, -OFF)) * 12.0 + ph)
                  + aw_sin(length(q + vec2( OFF,  OFF)) * 12.0 + ph)
                  + aw_sin(length(q + vec2(-OFF,  OFF)) * 12.0 + ph)) / 32.0;
        } else if (mode == 9) {
            return d + aw_sin(ang * 5.0 + ph) / 64.0;
        } else if (mode == 10) {
            return wx2 / 4.0 + wy2 / 4.0;
        } else if (mode == 11) {
            return wx2 / 8.0 + wy2 / 8.0;
        } else if (mode == 12) {
            return d + ph;
        } else if (mode == 13) {
            return ang;
        } else if (mode == 14) {
            return ang + aw_sin(d * 8.0 + ph) / 32.0;
        } else if (mode == 15) {
            return aw_sin(d * 4.0 + ph) / 32.0;
        } else if (mode == 16) {
            return d + aw_sin(d * 4.0 + ph) / 32.0;
        } else if (mode == 17) {
            return aw_sin(aw_cos(ax2 + ph)) / (20.0 + d)
                 + aw_sin(aw_cos(ay2 + ph)) / (20.0 + d);
        } else if (mode == 18) {
            return wx7 / (20.0 + d) + wy7 / (20.0 + d);
        } else if (mode == 19) {
            return wx17 / (20.0 + d) + wy17 / (20.0 + d);
        } else if (mode == 20) {
            return wx17 / 32.0 + wy17 / 32.0 + d + ang;
        } else if (mode == 21) {
            return wx7 / 32.0 + wy7 / 32.0 + d;
        } else if (mode == 22) {
            return (wx7 + wy7 + wx11 + wy11) / 32.0;
        } else if (mode == 23) {
            return aw_sin(ang * 7.0 + ph) / 32.0;
        } else if (mode == 24) {
            return (aw_sin(length(q + c0) * 2.0 + ph)
                  + aw_sin(length(q + c1) * 4.0 + ph)
                  + aw_sin(length(q + c2) * 6.0 + ph)
                  + aw_sin(length(q + c3) * 8.0 + ph)) / 12.0;
        } else if (mode == 25) {
            return ang*2.0 + (aw_sin(length(q + c0) * 2.0 + ph)
                            + aw_sin(length(q + c1) * 4.0 + ph)) / 16.0
                          + (aw_sin(length(q + c2) * 6.0 + ph)
                            + aw_sin(length(q + c3) * 8.0 + ph)) /  8.0;
        } else if (mode == 26) {
            return ang*4.0 + (aw_sin(length(q + c0) * 2.0 + ph)
                            + aw_sin(length(q + c1) * 4.0 + ph)
                            + aw_sin(length(q + c2) * 6.0 + ph)
                            + aw_sin(length(q + c3) * 8.0 + ph)) / 12.0;
        } else if (mode == 27) {
            return (aw_sin(length(q + c0) * 2.0 + ph)
                  + aw_sin(length(q + c1) * 4.0 + ph)
                  + aw_sin(length(q + c2) * 6.0 + ph)
                  + aw_sin(length(q + c3) * 8.0 + ph)) / 32.0;
        } else if (mode == 28) {
            return rain(q / res.y, 1.0 + ph * 0.01) * 255.0;
        } else if (mode == 29) {
            return d / 6.0 + rain(q / res.y, 2.0 + ph * 0.01) * 128.0;
        } else if (mode == 30) {
            return fxor(fxor(aw_sin(length(q + vec2(0.0, -OFF)) * 4.0 + ph) / 32.0,
                             aw_sin(length(q + vec2( OFF,  OFF)) * 4.0 + ph) / 32.0),
                             aw_sin(length(q + vec2(-OFF,  OFF)) * 4.0 + ph) / 32.0);
        } else if (mode == 31) {
            return fxor(mod(ang, QUART), d);
        } else if (mode == 32) {
            return fxor(pix.y - center.y, pix.x - center.x);
        } else if (mode == 33) {
            return rain(q / res.y, 3.0 + ph * 0.01) * 220.0;
        } else if (mode == 34) {
            return rain(q / res.y, 4.0 + ph * 0.01) * 255.0;
        } else if (mode == 35) {
            vec2 q2 = vec2(q.x, q.y * 2.0);
            return (ang + aw_sin(d * 8.0 + ph) / 32.0
                  + angle_of(q2) + aw_sin(length(q2) * 8.0 + ph) / 32.0) * 0.5;
        } else if (mode == 36) {
            vec2 q2 = vec2(q.x, q.y * 2.0);
            float head = ang + aw_sin(d * 10.0 + ph) / 16.0 + wx2 / 8.0 + wy2 / 8.0;
            return (head + angle_of(q2) + aw_sin(length(q2) * 8.0 + ph) / 32.0) * 0.5;
        } else if (mode == 37) {
            vec2 q2 = vec2(q.x, q.y * 2.0);
            float f1 = ang + aw_sin(d * 10.0 + ph) / 16.0 + wx2 / 8.0 + wy2 / 8.0;
            float f2 = angle_of(q2) + aw_sin(length(q2) * 10.0 + ph) / 16.0
                     + wx2 / 8.0 + wy2 / 8.0;
            return (f1 + f2) * 0.5;
        } else if (mode == 38) {
            int dyi = int(pix.y - center.y);
            vec2 qq = ((dyi & 1) != 0) ? vec2(q.x, q.y * 2.0) : q;
            return angle_of(qq) + aw_sin(length(qq) * 8.0 + ph) / 32.0;
        } else if (mode == 39) {
            vec2 q2 = vec2(q.x, q.y * 2.0);
            return (fxor(mod(ang, QUART), d)
                  + fxor(mod(angle_of(q2), QUART), length(q2))) * 0.5;
        } else if (mode == 40) {
            float dy = pix.y - center.y;
            float dx = pix.x - center.x;
            return (fxor(dy, dx) + fxor(dy * 2.0, dx)) * 0.5;
        }
        return hash21(pix + vec2(ph)) * 255.0;
    }

    void main() {
        ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
        if (pixel.x >= int(u_resolution.x) || pixel.y >= int(u_resolution.y))
            return;

        vec2 pix = vec2(pixel);
        vec2 center = u_resolution * 0.5;
        float ph = u_time * u_motion * 60.0;

        vec2 c0 = u_centers[0].xy;
        vec2 c1 = u_centers[0].zw;
        vec2 c2 = u_centers[1].xy;
        vec2 c3 = u_centers[1].zw;

        float c = field(pix, center, u_func, ph, c0, c1, c2, c3, u_resolution);
        float idx = fract(c / 255.0);

        // Per-channel colormap phase = R/G/B palette roll of the original
        vec3 col;
        if (abs(u_roll.x - u_roll.y) < 1e-6 && abs(u_roll.y - u_roll.z) < 1e-6) {
            col = colormap(fract(idx + u_roll.x)).rgb;
        } else {
            col = vec3(colormap(fract(idx + u_roll.x)).r,
                       colormap(fract(idx + u_roll.y)).g,
                       colormap(fract(idx + u_roll.z)).b);
        }

        // Sparkle: brighten every 4th quantized palette entry (1, 5, 9, ...)
        if (u_sparkle > 0.0) {
            float idx_int = floor(c / 255.0 * 256.0 + 0.5);
            float mod_val = mod(idx_int - 1.0, 4.0);
            float sparkle_hit = 1.0 - smoothstep(0.0, 1.0, mod_val);
            col = mix(col, vec3(1.0), sparkle_hit * u_sparkle);
        }

        // Fade toward u_fade.rgb by u_fade.a
        col = mix(col, u_fade.xyz, clamp(u_fade.w, 0.0, 1.0));

        imageStore(output_image, pixel, vec4(col, 1.0));
    }
)";

AcidWarpM3::AcidWarpM3()
    : Art("AcidWarp-minimaxm3") {
    useCompute();
    current_colormap_name = easel->pal.get_cmap().getTitle();
    init_shader();
    update_uniform_callback();
}

void AcidWarpM3::init_shader() {
    ecompute()->set_compute_shader(compute_shader_base + easel->pal.get_cmap().getSource());
}

void AcidWarpM3::update_uniform_callback() {
    ecompute()->set_uniform_callback([this](GLuint program) {
        GLint centers_loc = glGetUniformLocation(program, "u_centers");
        if (centers_loc >= 0)
            glUniform2fv(centers_loc, 4, centers);
        ecompute()->set_uniform_int("u_func", current_func);
        ecompute()->set_uniform_float("u_scale", pattern_scale);
        ecompute()->set_uniform_float("u_motion", motion_speed);
        ecompute()->set_uniform_vec3("u_roll", roll[0], roll[1], roll[2]);
        float t = fade_to_white ? 1.0f : 0.0f;
        ecompute()->set_uniform_vec4("u_fade", t, t, t, fade);
        ecompute()->set_uniform_float("u_sparkle", sparkle);
    });
}

void AcidWarpM3::pick_pattern() {
    current_func = (func_mode <= 40)
        ? func_mode
        : static_cast<int>(xoshiro256plus() % 41);
    for (int i = 0; i < 8; ++i) {
        // original RANDOM(40)-20 = [-20, 19] pixel offsets tuned for 320x200;
        // keep the same relative spread in unit-of-screen-height coordinates
        centers[i] = static_cast<float>(
            static_cast<long>(xoshiro256plus() % 40) - 20);
    }
}

bool AcidWarpM3::render(uint32_t *p) {
    ++frame;

    // per-channel palette roll: each channel flips direction with 1/256 prob
    for (int ch = 0; ch < 3; ++ch) {
        if (xoshiro256plus() % 256 == 0)
            roll_dir[ch] = -roll_dir[ch];
        roll[ch] += roll_dir[ch] * palette_speed / 256.0f;
        roll[ch] -= floorf(roll[ch]);
    }

    switch (state) {
    case IMAGE:
        pick_pattern();
        state = FADE_IN;
        frame = 0;
        // fallthrough, like the original
    case FADE_IN:
        // 6-bit VGA palette faded one step per frame - at most 63 frames
        fade = std::max(0.0f, 1.0f - frame / 63.0f);
        if (frame > frames_per_state)
            frame = 0, state = ROTATE;
        break;
    case ROTATE:
        fade = 0.0f;
        if (frame > frames_per_state)
            frame = 0, state = FADE_OUT;
        break;
    case FADE_OUT:
        fade = std::min(1.0f, frame / 63.0f);
        if (frame > frames_per_state)
            frame = 0, state = IMAGE;
        break;
    }

    return true;
}

bool AcidWarpM3::render_gui() {
    bool re = false;

    std::string new_colormap = easel->pal.get_cmap().getTitle();
    if (new_colormap != current_colormap_name) {
        current_colormap_name = new_colormap;
        init_shader();
    }

    re |= ScrollableSliderInt("image func", &func_mode, 0, 41, "%d", 1);
    ScrollableSliderInt("frames each state", &frames_per_state, 63, 60 * 300, "%d", 60);
    ScrollableSliderFloat("pattern scale", &pattern_scale, 0.25f, 4.0f, "%.2f", 0.05f);
    ScrollableSliderFloat("motion", &motion_speed, 0.0f, 2.0f, "%.2f", 0.02f);
    ScrollableSliderFloat("palette speed", &palette_speed, 0.0f, 8.0f, "%.2f", 0.1f);
    ScrollableSliderFloat("sparkle", &sparkle, 0.0f, 1.0f, "%.2f", 0.02f);
    ImGui::Checkbox("fade to white", &fade_to_white);

    if (re) {
        frame = 0;
        state = IMAGE;
        pick_pattern();
    }

    ImGui::Text("state:%d frame:%d func:%d", state, frame, current_func);

    return false;
}

void AcidWarpM3::resize(int _w, int _h) {
    default_resize(_w, _h);
    frame = 0;
    state = IMAGE;
    fade = 1.0f;
}
