/* GPU AcidWarp interpretation by Noah Spurrier, optimized for cloudlife. */

#include "acidwarpgpt56luna.h"

#include "easelcompute.h"
#include "imgui.h"
#include "imgui_elements.h"
#include "random.h"

#include <algorithm>
#include <cmath>

const char* AcidWarpGpt56Luna::compute_shader_base = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;
layout(rgba8, binding = 0) uniform writeonly image2D output_image;

uniform vec2 u_resolution;
uniform float u_time;
uniform int u_function;
uniform float u_scale;
uniform float u_motion;
uniform vec4 u_centers[2];
uniform float u_palette_phase;
uniform vec4 u_fade;

vec4 colormap(float x);
const float TAU = 6.28318530718;

float wave(float x) { return sin(x * TAU); }
float ring(vec2 p, float frequency) { return wave(length(p) * frequency); }
float angle01(vec2 p) { return atan(p.y, p.x) / TAU + 0.5; }
float hash21(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }

float centers_rings(vec2 p, float first_frequency) {
    return (ring(p + u_centers[0].xy, first_frequency) +
            ring(p + u_centers[0].zw, first_frequency * 2.0) +
            ring(p + u_centers[1].xy, first_frequency * 4.0) +
            ring(p + u_centers[1].zw, first_frequency * 8.0)) * 0.25;
}

float field(vec2 p, vec2 pixel, float time) {
    float r = length(p);
    float a = angle01(p);
    float wx = cos(p.x * 40.0 + time);
    float wy = cos(p.y * 40.0 - time * 0.73);
    float c = 0.0;
    int f = u_function;

    if (f == 0) c = a + ring(p, 32.0) * 0.17 + (wx + wy) * 0.12;
    else if (f == 1) c = a + ring(p, 32.0) * 0.44 + (wx + wy) * 0.31;
    else if (f == 2) c = centers_rings(p, 10.0);
    else if (f == 3) c = a * 2.0 + ring(p + vec2(0.20, 0.0), 28.0) * 0.5;
    else if (f == 4) c = ring(p, 14.0);
    else if (f == 5) c = cos(p.x * 20.0 + time) * 0.125 + cos(p.y * 20.0 + time) * 0.125 + a + ring(p, 20.0) * 0.03125;
    else if (f == 6 || f == 7 || f == 8) {
        float peacock = (ring(p + vec2(0.0, 0.20), 24.0) + ring(p + vec2(0.20, -0.20), 24.0) + ring(p + vec2(-0.20, -0.20), 24.0)) / 3.0;
        c = peacock * (f == 6 ? 1.0 : (f == 7 ? 1.0 : 1.4)) + (f == 7 ? a : 0.0);
    } else if (f == 9) c = r * 8.0 + sin(a * TAU * 5.0 + time) * 0.25;
    else if (f == 10) c = cos(p.x * 40.0 + time) * 0.25 + cos(p.y * 40.0 + time) * 0.25;
    else if (f == 11) c = cos(p.x * 20.0 + time) * 0.125 + cos(p.y * 20.0 + time) * 0.125;
    else if (f == 12) c = r * 12.0;
    else if (f == 13) c = a * 8.0;
    else if (f == 14) c = a * 7.0 + ring(p, 48.0) * 0.2;
    else if (f == 15) c = ring(p, 27.0);
    else if (f == 16) c = r * 10.0 + ring(p, 28.0) * 0.35;
    else if (f == 17) c = (sin(cos(p.x * 12.0)) + sin(cos(p.y * 12.0))) / (0.25 + r);
    else if (f == 18 || f == 19) c = (cos(p.x * (f == 18 ? 44.0 : 107.0)) + cos(p.y * (f == 18 ? 44.0 : 107.0))) / (0.25 + r);
    else if (f == 20) c = wx * 0.2 + wy * 0.2 + r * 8.0 + a;
    else if (f == 21) c = cos(p.x * 44.0) * 0.2 + cos(p.y * 44.0) * 0.2 + r * 8.0;
    else if (f == 22) c = cos(p.x * 44.0) + cos(p.y * 44.0) + cos(p.x * 69.0) + cos(p.y * 69.0);
    else if (f == 23) c = sin(a * TAU * 7.0 + time);
    else if (f >= 24 && f <= 27) c = centers_rings(p, 5.0 + float(f - 24) * 2.0) * (f == 27 ? 0.5 : 1.0) + (f >= 25 ? a : 0.0);
    else if (f == 28 || f == 29 || f == 33 || f == 34) {
        // The original rain modes are scanline-recursive; this decorrelated streak field is parallel.
        vec2 d = vec2(dot(p, vec2(0.7071, 0.7071)), dot(p, vec2(-0.7071, 0.7071)));
        c = hash21(floor(d * vec2(90.0, 180.0)) + floor(time * 0.05)) + (f == 29 ? r : 0.0);
    } else if (f == 30) c = sin(length(p + vec2(0.0, 0.20)) * 24.0) * sin(length(p + vec2(0.20, -0.20)) * 24.0);
    else if (f == 31) c = fract(a * 4.0) + r * 8.0;
    else if (f == 32) c = fract(abs(p.x) * 17.0) + fract(abs(p.y) * 17.0);
    else if (f == 35 || f == 36 || f == 37) {
        vec2 q = vec2(p.x, p.y * 2.0);
        float head = f == 35 ? a + ring(p, 32.0) * 0.25 : a + ring(p, 32.0) * 0.44 + (wx + wy) * 0.15;
        c = (head + angle01(q) + ring(q, f == 37 ? 40.0 : 32.0) * 0.25) * 0.5;
    } else if (f == 38) c = angle01((int(pixel.y) & 1) != 0 ? vec2(p.x, p.y * 2.0) : p) + ring(p, 48.0) * 0.2;
    else if (f == 39) c = (fract(a * 4.0) + r * 8.0 + fract(angle01(vec2(p.x, p.y * 2.0)) * 4.0)) * 0.5;
    else if (f == 40) c = fract(abs(p.x * 17.0)) + fract(abs(p.y * 17.0));
    else c = hash21(pixel + vec2(time));
    return c;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(u_resolution.x) || pixel.y >= int(u_resolution.y)) return;
    vec2 center = u_resolution * 0.5;
    float radius = min(u_resolution.x, u_resolution.y) * 0.5;
    vec2 p = (vec2(pixel) + 0.5 - center) / radius * u_scale;
    float index = fract(field(p, vec2(pixel), u_time * u_motion) + u_palette_phase);
    vec3 color = colormap(index).rgb;
    color = mix(color, u_fade.rgb, clamp(u_fade.a, 0.0, 1.0));
    imageStore(output_image, pixel, vec4(color, 1.0));
}
)";

AcidWarpGpt56Luna::AcidWarpGpt56Luna() : Art("AcidWarp-gpt56-luna") {
    useCompute();
    current_colormap_name = easel->pal.get_cmap().getTitle();
    init_shader();
    update_uniform_callback();
}

void AcidWarpGpt56Luna::init_shader() {
    ecompute()->set_compute_shader(compute_shader_base + easel->pal.get_cmap().getSource());
}

void AcidWarpGpt56Luna::update_uniform_callback() {
    ecompute()->set_uniform_callback([this](GLuint) {
        ecompute()->set_uniform_int("u_function", current_function);
        ecompute()->set_uniform_float("u_scale", pattern_scale);
        ecompute()->set_uniform_float("u_motion", motion_speed);
        ecompute()->set_uniform_float("u_palette_phase", palette_phase);
        ecompute()->set_uniform_vec4("u_centers[0]", centers[0], centers[1], centers[2], centers[3]);
        ecompute()->set_uniform_vec4("u_centers[1]", centers[4], centers[5], centers[6], centers[7]);
        const float fade_color = fade_to_white ? 1.0f : 0.0f;
        ecompute()->set_uniform_vec4("u_fade", fade_color, fade_color, fade_color, fade);
    });
}

void AcidWarpGpt56Luna::choose_pattern() {
    current_function = image_function <= 40 ? image_function : static_cast<int>(xoshiro256plus() % 41);
    for (float& center : centers)
        center = (static_cast<int>(xoshiro256plus() % 40) - 20) / 160.0f;
}

bool AcidWarpGpt56Luna::render(uint32_t*) {
    ++frame;
    palette_phase = std::fmod(palette_phase + palette_speed / 256.0f, 1.0f);
    switch (state) {
    case IMAGE:
        choose_pattern();
        state = FADE_IN;
        frame = 0;
        [[fallthrough]];
    case FADE_IN:
        fade = std::max(0.0f, 1.0f - frame / 63.0f);
        if (frame > frames_each_state) frame = 0, state = ROTATE;
        break;
    case ROTATE:
        fade = 0.0f;
        if (frame > frames_each_state) frame = 0, state = FADE_OUT;
        break;
    case FADE_OUT:
        fade = std::min(1.0f, frame / 63.0f);
        if (frame > frames_each_state) frame = 0, state = IMAGE;
        break;
    }
    return true;
}

bool AcidWarpGpt56Luna::render_gui() {
    bool reset = false;
    if (easel->pal.get_cmap().getTitle() != current_colormap_name) {
        current_colormap_name = easel->pal.get_cmap().getTitle();
        init_shader();
    }
    reset |= ScrollableSliderInt("image function", &image_function, 0, 40, "%d", 1);
    ScrollableSliderInt("frames each state", &frames_each_state, 63, 60 * 300, "%d", 60);
    ScrollableSliderFloat("pattern scale", &pattern_scale, 0.25f, 4.0f, "%.2f", 0.05f);
    ScrollableSliderFloat("motion", &motion_speed, 0.0f, 2.0f, "%.2f", 0.02f);
    ScrollableSliderFloat("palette speed", &palette_speed, 0.0f, 8.0f, "%.2f", 0.1f);
    ImGui::Checkbox("fade to white", &fade_to_white);
    if (reset) { frame = 0; state = IMAGE; }
    ImGui::Text("state:%d frame:%d function:%d", state, frame, current_function);
    return false;
}

void AcidWarpGpt56Luna::resize(int width, int height) {
    default_resize(width, height);
    frame = 0;
    state = IMAGE;
    fade = 1.0f;
}
