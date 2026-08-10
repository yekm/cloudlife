/* AcidWarp-qwen38-max - GPU rework of the classic AcidWarp hack
 * (Noah Spurrier, https://noah.org/acidwarp/).
 *
 * The original port (acidwarp.cpp) generates an indexed 256 color image on the
 * CPU every frame, then animates a 256x3 VGA palette (roll + fade state
 * machine) and converts every pixel through the palette on the CPU.
 *
 * This version does everything in a compute shader:
 *   - all 41 image functions of generate_image() are evaluated per pixel on
 *     the GPU, converted from the integer LUT math (lut.c: ANGLE_UNIT=256,
 *     sine amplitude +-511) to floating point;
 *   - the VGA palette is replaced by colormap-shaders colormaps appended to
 *     the shader source (see easel->pal);
 *   - the independent R/G/B palette roll of roll_rgb_palArray() is emulated
 *     as per channel colormap phase offsets - the colormap is sampled up to
 *     three times with different offsets;
 *   - the IMAGE/FADE_IN/ROTATE/FADE_OUT state machine survives as an O(1)
 *     CPU side driver of a single vec4 fade uniform.
 *
 * CPU cost per frame is constant: state machine, palette roll phases and a
 * handful of uniforms. No per pixel work, no palette arrays, no upload.
 *
 * Cases 28/29/33/34 of the original are recursive (each pixel depends on its
 * left and upper neighbours) and cannot be evaluated in parallel; they are
 * approximated with diagonal streak noise.
 */

#include "acidwarpqwen38max.h"

#include "easelcompute.h"
#include "imgui.h"
#include "imgui_elements.h"
#include "random.h"

#include <algorithm>
#include <cmath>

const char *AcidWarpQwen38Max::compute_shader_base = R"(
    #version 430 core

    layout(local_size_x = 16, local_size_y = 16) in;

    layout(rgba8, binding = 0) uniform writeonly image2D output_image;

    uniform vec2 u_resolution;
    uniform float u_time;
    uniform int u_func;            // image function 0..41
    uniform float u_scale;         // pattern density
    uniform float u_motion;        // 0 = static like the original
    uniform vec2 u_centers[4];     // random centers for cases 2, 24..27
    uniform vec3 u_roll;           // per channel colormap phase
    uniform vec4 u_fade;           // xyz = fade color, w = fade amount

    // Colormap function, injected from colormap-shaders
    vec4 colormap(float x);

    const float TAU = 6.283185307179586;
    const float AU = 256.0;        // original ANGLE_UNIT
    const float TA = 511.0;        // original TRIG_UNIT sine amplitude
    const float OFF = 0.2;         // original fixed 20px peacock centers, normalized

    float aw_sin(float a) { return TA * sin(TAU * a / AU); }
    float aw_cos(float a) { return TA * cos(TAU * a / AU); }

    float angle_of(vec2 v) {
        return mod(atan(v.y, v.x) / TAU, 1.0) * AU;
    }

    // normalized "pixel" distance, scaled like the original 1024 wide frames
    float dist_of(vec2 v) {
        return length(v) * 512.0 * u_scale;
    }

    float hash21(vec2 p) {
        return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
    }

    // integer XOR of the original on quantized color values
    float fxor(float a, float b) {
        int ia = int(mod(a, 256.0));
        int ib = int(mod(b, 256.0));
        return float(ia ^ ib);
    }

    // approximation of the recursive "rain" cases 28/29/33/34
    float rain(vec2 q, float seed) {
        vec2 d = vec2((q.x + q.y) * 0.70710678, (q.y - q.x) * 0.70710678);
        float streak = hash21(vec2(floor(d.x * 48.0), seed));
        float grain = hash21(floor(d * vec2(48.0, 96.0)) + vec2(seed));
        return streak * 0.6 + grain * 0.4;
    }

    void main() {
        ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
        if (pixel.x >= int(u_resolution.x) || pixel.y >= int(u_resolution.y))
            return;

        vec2 pix = vec2(pixel);
        vec2 center = u_resolution * 0.5;
        float half_min = min(u_resolution.x, u_resolution.y) * 0.5;
        vec2 q = (pix - center) / half_min;      // half min dimension = 1.0
        vec2 uv = pix / u_resolution;
        vec2 w = uv * AU;                        // wave coords in angle units
        float d = dist_of(q);
        float ang = angle_of(q);
        float ph = u_time * u_motion * 60.0;     // optional animation phase
        int mode = u_func;

        float c = 0.0;

        if (mode == 0) {          /* Rays plus 2D Waves */
            c = ang + aw_sin(d*10.0 + ph)/64.0 + aw_cos(w.x*2.0 + ph)/32.0 + aw_cos(w.y*2.0 + ph)/32.0;
        } else if (mode == 1) {   /* Rays plus 2D Waves (stronger) */
            c = ang + aw_sin(d*10.0 + ph)/16.0 + aw_cos(w.x*2.0 + ph)/8.0 + aw_cos(w.y*2.0 + ph)/8.0;
        } else if (mode == 2) {
            c = (aw_sin(dist_of(q + u_centers[0])*4.0 + ph) + aw_sin(dist_of(q + u_centers[1])*8.0 + ph)
               + aw_sin(dist_of(q + u_centers[2])*16.0 + ph) + aw_sin(dist_of(q + u_centers[3])*32.0 + ph)) / 32.0;
        } else if (mode == 3) {   /* Peacock */
            c = ang*2.0 + (aw_sin(dist_of(q + vec2(OFF, 0.0))*10.0 + ph)
                         + aw_sin(dist_of(q - vec2(OFF, 0.0))*10.0 + ph)) / 32.0;
        } else if (mode == 4) {
            c = aw_sin(d + ph)/16.0;
        } else if (mode == 5) {   /* 2D Wave + Spiral */
            c = aw_cos(w.x + ph)/8.0 + aw_cos(w.y + ph)/8.0 + ang + aw_sin(d + ph)/32.0;
        } else if (mode == 6) {   /* Peacock, three centers */
            c = (aw_sin(dist_of(q - vec2(0.0, OFF))*4.0 + ph)
               + aw_sin(dist_of(q + vec2(OFF, -OFF))*4.0 + ph)
               + aw_sin(dist_of(q - vec2(OFF, -OFF))*4.0 + ph)) / 32.0;
        } else if (mode == 7) {
            c = ang + (aw_sin(dist_of(q - vec2(0.0, OFF))*8.0 + ph)
                     + aw_sin(dist_of(q + vec2(OFF, -OFF))*8.0 + ph)
                     + aw_sin(dist_of(q - vec2(OFF, -OFF))*8.0 + ph)) / 32.0;
        } else if (mode == 8) {
            c = (aw_sin(dist_of(q - vec2(0.0, OFF))*12.0 + ph)
               + aw_sin(dist_of(q + vec2(OFF, -OFF))*12.0 + ph)
               + aw_sin(dist_of(q - vec2(OFF, -OFF))*12.0 + ph)) / 32.0;
        } else if (mode == 9) {   /* Five Arm Star */
            c = d + aw_sin(ang*5.0 + ph)/64.0;
        } else if (mode == 10) {  /* 2D Wave */
            c = aw_cos(w.x*2.0 + ph)/4.0 + aw_cos(w.y*2.0 + ph)/4.0;
        } else if (mode == 11) {  /* 2D Wave */
            c = aw_cos(w.x + ph)/8.0 + aw_cos(w.y + ph)/8.0;
        } else if (mode == 12) {  /* Simple Concentric Rings */
            c = d + ph;
        } else if (mode == 13) {  /* Simple Rays */
            c = ang;
        } else if (mode == 14) {  /* Toothed Spiral Sharp */
            c = ang + aw_sin(d*8.0 + ph)/32.0;
        } else if (mode == 15) {  /* Rings with sine */
            c = aw_sin(d*4.0 + ph)/32.0;
        } else if (mode == 16) {  /* Rings with sliding inner rings */
            c = d + aw_sin(d*4.0 + ph)/32.0;
        } else if (mode == 17) {
            c = aw_sin(aw_cos(w.x*2.0 + ph))/(20.0 + d) + aw_sin(aw_cos(w.y*2.0 + ph))/(20.0 + d);
        } else if (mode == 18) {  /* 2D Wave */
            c = (aw_cos(w.x*7.0 + ph) + aw_cos(w.y*7.0 + ph))/(20.0 + d);
        } else if (mode == 19) {  /* 2D Wave */
            c = (aw_cos(w.x*17.0 + ph) + aw_cos(w.y*17.0 + ph))/(20.0 + d);
        } else if (mode == 20) {  /* 2D Wave Interference */
            c = aw_cos(w.x*17.0 + ph)/32.0 + aw_cos(w.y*17.0 + ph)/32.0 + d + ang;
        } else if (mode == 21) {  /* 2D Wave Interference */
            c = aw_cos(w.x*7.0 + ph)/32.0 + aw_cos(w.y*7.0 + ph)/32.0 + d;
        } else if (mode == 22) {  /* 2D Wave Interference */
            c = (aw_cos(w.x*7.0 + ph) + aw_cos(w.y*7.0 + ph)
               + aw_cos(w.x*11.0 + ph) + aw_cos(w.y*11.0 + ph)) / 32.0;
        } else if (mode == 23) {
            c = aw_sin(ang*7.0 + ph)/32.0;
        } else if (mode == 24) {
            c = (aw_sin(dist_of(q + u_centers[0])*2.0 + ph) + aw_sin(dist_of(q + u_centers[1])*4.0 + ph)
               + aw_sin(dist_of(q + u_centers[2])*6.0 + ph) + aw_sin(dist_of(q + u_centers[3])*8.0 + ph)) / 12.0;
        } else if (mode == 25) {
            c = ang*2.0 + (aw_sin(dist_of(q + u_centers[0])*2.0 + ph)
                         + aw_sin(dist_of(q + u_centers[1])*4.0 + ph)) / 16.0
                        + (aw_sin(dist_of(q + u_centers[2])*6.0 + ph)
                         + aw_sin(dist_of(q + u_centers[3])*8.0 + ph)) / 8.0;
        } else if (mode == 26) {
            c = ang*4.0 + (aw_sin(dist_of(q + u_centers[0])*2.0 + ph)
                         + aw_sin(dist_of(q + u_centers[1])*4.0 + ph)
                         + aw_sin(dist_of(q + u_centers[2])*6.0 + ph)
                         + aw_sin(dist_of(q + u_centers[3])*8.0 + ph)) / 12.0;
        } else if (mode == 27) {
            c = (aw_sin(dist_of(q + u_centers[0])*2.0 + ph) + aw_sin(dist_of(q + u_centers[1])*4.0 + ph)
               + aw_sin(dist_of(q + u_centers[2])*6.0 + ph) + aw_sin(dist_of(q + u_centers[3])*8.0 + ph)) / 32.0;
        } else if (mode == 28) {  /* Random Curtain of Rain - approximated */
            c = rain(q, 1.0 + ph*0.01) * 255.0;
        } else if (mode == 29) {
            c = d/6.0 + rain(q, 2.0 + ph*0.01) * 128.0;
        } else if (mode == 30) {
            c = fxor(fxor(aw_sin(dist_of(q - vec2(0.0, OFF))*4.0 + ph)/32.0,
                          aw_sin(dist_of(q + vec2(OFF, -OFF))*4.0 + ph)/32.0),
                          aw_sin(dist_of(q - vec2(OFF, -OFF))*4.0 + ph)/32.0);
        } else if (mode == 31) {
            c = fxor(mod(ang, AU/4.0), d);
        } else if (mode == 32) {
            c = fxor(pix.y - center.y, pix.x - center.x);
        } else if (mode == 33) {  /* Variation on Rain - approximated */
            c = rain(q, 3.0 + ph*0.01) * 220.0;
        } else if (mode == 34) {  /* Variation on Rain - approximated */
            c = rain(q, 4.0 + ph*0.01) * 255.0;
        } else if (mode == 35) {
            vec2 q2 = vec2(q.x, q.y*2.0);
            c = ((ang + aw_sin(d*8.0 + ph)/32.0)
               + (angle_of(q2) + aw_sin(dist_of(q2)*8.0 + ph)/32.0)) / 2.0;
        } else if (mode == 36) {
            vec2 q2 = vec2(q.x, q.y*2.0);
            float head = ang + aw_sin(d*10.0 + ph)/16.0 + aw_cos(w.x*2.0 + ph)/8.0 + aw_cos(w.y*2.0 + ph)/8.0;
            c = (head + angle_of(q2) + aw_sin(dist_of(q2)*8.0 + ph)/32.0) / 2.0;
        } else if (mode == 37) {
            vec2 q2 = vec2(q.x, q.y*2.0);
            float f1 = ang + aw_sin(d*10.0 + ph)/16.0 + aw_cos(w.x*2.0 + ph)/8.0 + aw_cos(w.y*2.0 + ph)/8.0;
            float f2 = angle_of(q2) + aw_sin(dist_of(q2)*10.0 + ph)/16.0
                     + aw_cos(w.x*2.0 + ph)/8.0 + aw_cos(w.y*2.0 + ph)/8.0;
            c = (f1 + f2) / 2.0;
        } else if (mode == 38) {
            int dyi = int(pix.y - center.y);
            vec2 qq = ((dyi & 1) != 0) ? vec2(q.x, q.y*2.0) : q;
            c = angle_of(qq) + aw_sin(dist_of(qq)*8.0 + ph)/32.0;
        } else if (mode == 39) {
            vec2 q2 = vec2(q.x, q.y*2.0);
            c = (fxor(mod(ang, AU/4.0), d)
               + fxor(mod(angle_of(q2), AU/4.0), dist_of(q2))) / 2.0;
        } else if (mode == 40) {
            float dy = pix.y - center.y;
            float dx = pix.x - center.x;
            c = (fxor(dy, dx) + fxor(dy*2.0, dx)) / 2.0;
        } else {                  /* original default: random noise */
            c = hash21(pix + vec2(ph)) * 255.0;
        }

        float idx = fract(c / 255.0);

        /* per channel colormap phase = R/G/B palette roll of the original */
        vec3 col;
        if (abs(u_roll.x - u_roll.y) < 1e-6 && abs(u_roll.y - u_roll.z) < 1e-6) {
            col = colormap(fract(idx + u_roll.x)).rgb;
        } else {
            col = vec3(colormap(fract(idx + u_roll.x)).r,
                       colormap(fract(idx + u_roll.y)).g,
                       colormap(fract(idx + u_roll.z)).b);
        }

        col = mix(col, u_fade.xyz, clamp(u_fade.w, 0.0, 1.0));

        imageStore(output_image, pixel, vec4(col, 1.0));
    }
)";

AcidWarpQwen38Max::AcidWarpQwen38Max()
    : Art("AcidWarp-qwen38-max") {
    useCompute();
    current_colormap_name = easel->pal.get_cmap().getTitle();
    init_shader();
    update_uniform_callback();
}

void AcidWarpQwen38Max::init_shader() {
    ecompute()->set_compute_shader(compute_shader_base + easel->pal.get_cmap().getSource());
}

void AcidWarpQwen38Max::update_uniform_callback() {
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
    });
}

void AcidWarpQwen38Max::pick_pattern() {
    current_func = (func_mode <= 40)
        ? func_mode
        : static_cast<int>(xoshiro256plus() % 41);
    for (int i = 0; i < 8; ++i) {
        /* original used RANDOM(40)-20 pixel offsets tuned for 320x200 screens;
         * 160 was half of that width, keep the same relative spread */
        centers[i] = static_cast<float>(
            static_cast<long>(xoshiro256plus() % 40) - 20) / 160.0f;
    }
}

bool AcidWarpQwen38Max::render(uint32_t *p) {
    ++frame;

    /* original rolls the palette one entry per frame, each channel rolls in
     * its own direction that inverts with probability 1/256 per frame */
    for (int ch = 0; ch < 3; ++ch) {
        if (xoshiro256plus() % 256 == 0)
            roll_dir[ch] = -roll_dir[ch];
        roll[ch] += roll_dir[ch] * roll_speed / 256.0f;
        roll[ch] -= floorf(roll[ch]);
    }

    switch (acid_state) {
    case IMAGE:
        pick_pattern();
        acid_state = FADE_IN;
        // fallthrough, like the original
    case FADE_IN:
        /* 6-bit VGA palette faded one step per frame - at most 63 frames */
        fade = std::max(0.0f, 1.0f - frame / 63.0f);
        if (frame > frame_max)
            frame = 0, acid_state = ROTATE;
        break;
    case ROTATE:
        fade = 0.0f;
        if (frame > frame_max)
            frame = 0, acid_state = FADE_OUT;
        break;
    case FADE_OUT:
        fade = std::min(1.0f, frame / 63.0f);
        if (frame > frame_max)
            frame = 0, acid_state = IMAGE;
        break;
    }

    return true;
}

bool AcidWarpQwen38Max::render_gui() {
    bool re = false;

    std::string new_colormap = easel->pal.get_cmap().getTitle();
    if (new_colormap != current_colormap_name) {
        current_colormap_name = new_colormap;
        init_shader();
    }

    re |= ScrollableSliderInt("image func", &func_mode, 0, 40, "%d", 1);
    ScrollableSliderInt("frames each state", &frame_max, 63, 60 * 300, "%d", 60);
    ScrollableSliderFloat("pattern scale", &pattern_scale, 0.25f, 4.0f, "%.2f", 0.05f);
    ScrollableSliderFloat("motion", &motion_speed, 0.0f, 2.0f, "%.2f", 0.02f);
    ScrollableSliderFloat("roll speed", &roll_speed, 0.0f, 8.0f, "%.2f", 0.1f);
    ImGui::Checkbox("fade to white", &fade_to_white);

    if (re) {
        frame = 0;
        acid_state = IMAGE;
        pick_pattern();
    }

    ImGui::Text("state:%d frame:%d func:%d", acid_state, frame, current_func);

    return false;
}

void AcidWarpQwen38Max::resize(int _w, int _h) {
    default_resize(_w, _h);
    frame = 0;
    acid_state = IMAGE;
    fade = 1.0f;
}
