/*
 * GPU interpretation of AcidWarp by Noah Spurrier.
 * The original port generates an indexed image and converts it through a
 * CPU-side palette every frame. This version evaluates the same families of
 * radial, angular, and wave fields directly in a compute shader.
 */

#include "acidwarpgpt56terra.h"

#include "easelcompute.h"
#include "imgui_elements.h"

const char* AcidWarpGpt56Terra::compute_shader_base = R"(
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

layout(rgba8, binding = 0) uniform writeonly image2D output_image;

uniform vec2 u_resolution;
uniform float u_time;
uniform int u_image_function;
uniform float u_pattern_scale;
uniform float u_motion_speed;
uniform float u_palette_speed;

vec4 colormap(float x);

const float TAU = 6.28318530718;

float rings(vec2 point, float frequency, float time) {
    return sin(length(point) * frequency - time);
}

float peacock(vec2 point, float time) {
    float a = rings(point + vec2(0.0, 0.32), 24.0, time);
    float b = rings(point + vec2(0.28, -0.20), 24.0, time * 1.13);
    float c = rings(point + vec2(-0.28, -0.20), 24.0, time * 0.91);
    return (a + b + c) / 3.0;
}

float multi_center(vec2 point, float time) {
    vec2 c0 = vec2(sin(time * 0.71), cos(time * 0.57)) * 0.25;
    vec2 c1 = vec2(cos(time * 0.49), sin(time * 0.83)) * 0.25;
    vec2 c2 = vec2(sin(time * 0.37 + 2.0), cos(time * 0.63 + 1.0)) * 0.25;
    vec2 c3 = vec2(cos(time * 0.91 + 3.0), sin(time * 0.43 + 2.0)) * 0.25;
    return (rings(point + c0, 11.0, time) + rings(point + c1, 19.0, time) +
            rings(point + c2, 27.0, time) + rings(point + c3, 35.0, time)) * 0.25;
}

float hash21(vec2 point) {
    return fract(sin(dot(point, vec2(127.1, 311.7))) * 43758.5453123);
}

float acid_field(vec2 point, float time, int image_function) {
    float radius = length(point);
    float angle = atan(point.y, point.x) / TAU;
    float wave_x = cos(point.x * TAU * 4.0 + time);
    float wave_y = cos(point.y * TAU * 4.0 - time * 0.73);
    int mode = image_function % 41;

    if (mode == 0) return angle + rings(point, 32.0, time) * 0.17 + (wave_x + wave_y) * 0.12;
    if (mode == 1) return angle + rings(point, 32.0, time) * 0.44 + (wave_x + wave_y) * 0.31;
    if (mode == 2) return multi_center(point, time);
    if (mode == 3) return angle * 2.0 + rings(point + vec2(0.25, 0.0), 28.0, time) * 0.5;
    if (mode == 4) return rings(point, 14.0, time);
    if (mode == 5) return angle + rings(point, 18.0, time) * 0.33 + (wave_x + wave_y) * 0.2;
    if (mode == 6) return peacock(point, time);
    if (mode == 7) return angle + peacock(point, time) * 0.75;
    if (mode == 8) return peacock(point, time * 1.6) + rings(point, 43.0, time) * 0.2;
    if (mode == 9) return radius * 6.0 + sin(angle * 5.0 + time) * 0.28;
    if (mode == 10) return wave_x + wave_y;
    if (mode == 11) return cos(point.x * TAU * 2.0 + time) + cos(point.y * TAU * 2.0 - time);
    if (mode == 12) return radius * 11.0;
    if (mode == 13) return angle * 8.0;
    if (mode == 14) return angle * 7.0 + rings(point, 48.0, time) * 0.2;
    if (mode == 15) return rings(point, 27.0, time);
    if (mode == 16) return radius * 10.0 + rings(point, 28.0, time) * 0.35;
    if (mode == 17) return (sin(cos(point.x * 12.0)) + sin(cos(point.y * 12.0))) / (0.25 + radius);
    if (mode == 18) return (cos(point.x * 44.0) + cos(point.y * 44.0)) / (0.25 + radius);
    if (mode == 19) return (cos(point.x * 107.0) + cos(point.y * 107.0)) / (0.25 + radius);
    if (mode == 20) return wave_x * 0.2 + wave_y * 0.2 + radius * 8.0 + angle;
    if (mode == 21) return cos(point.x * 44.0) * 0.2 + cos(point.y * 44.0) * 0.2 + radius * 8.0;
    if (mode == 22) return cos(point.x * 44.0) + cos(point.y * 44.0) + cos(point.x * 69.0) + cos(point.y * 69.0);
    if (mode == 23) return sin(angle * 7.0 + time);
    if (mode == 24) return multi_center(point, time * 0.5) * 1.8;
    if (mode == 25) return angle * 1.7 + multi_center(point, time) * 1.5;
    if (mode == 26) return angle * 3.2 + multi_center(point, time * 1.3) * 1.4;
    if (mode == 27) return multi_center(point, time * 1.9) + rings(point, 9.0, time) * 0.2;
    if (mode == 28) return point.x * 10.0 + point.y * 18.0 + hash21(floor(point * 80.0) + vec2(time)) * 2.0;
    if (mode == 29) return radius * 3.0 + point.x * 9.0 + point.y * 13.0 + hash21(floor(point * 96.0) + vec2(time)) * 2.0;
    if (mode == 30) return sin(length(point + vec2(0.0, 0.32)) * 24.0) * sin(length(point + vec2(0.28, -0.20)) * 24.0) * sin(length(point + vec2(-0.28, -0.20)) * 24.0);
    if (mode == 31) return fract(angle * 4.0) + radius * 9.0;
    if (mode == 32) return fract(abs(point.x) * 17.0) + fract(abs(point.y) * 17.0);
    if (mode == 33) return point.x * 8.0 + point.y * 8.0 + hash21(floor(point * 120.0)) * 0.3;
    if (mode == 34) return point.x * 8.0 + point.y * 8.0 + hash21(floor(point * 80.0)) * 0.7;
    if (mode == 35) return (angle + rings(point, 32.0, time) * 0.25 + atan(point.y * 2.0, point.x) / TAU + rings(point * vec2(1.0, 2.0), 32.0, time) * 0.25) * 0.5;
    if (mode == 36) return (angle + rings(point, 32.0, time) * 0.44 + wave_x * 0.31 + atan(point.y * 2.0, point.x) / TAU + rings(point * vec2(1.0, 2.0), 48.0, time) * 0.2) * 0.5;
    if (mode == 37) return (angle + rings(point, 32.0, time) * 0.44 + wave_x * 0.31 + atan(point.y * 2.0, point.x) / TAU + rings(point * vec2(1.0, 2.0), 32.0, time) * 0.44) * 0.5;
    if (mode == 38) return angle + rings(point * vec2(1.0, 2.0), 48.0, time) * 0.2;
    if (mode == 39) return (fract(angle * 4.0) + radius * 9.0 + fract(atan(point.y * 2.0, point.x) / TAU * 4.0) + length(point * vec2(1.0, 2.0)) * 9.0) * 0.5;
    return (fract(abs(point.x) * 17.0) + fract(abs(point.y) * 17.0) + fract(abs(point.x) * 17.0) + fract(abs(point.y * 2.0) * 17.0)) * 0.5;
}

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(u_resolution.x) || pixel.y >= int(u_resolution.y)) return;

    vec2 uv = (vec2(pixel) + 0.5) / u_resolution - 0.5;
    uv.x *= u_resolution.x / u_resolution.y;
    float time = u_time * u_motion_speed;
    float field = acid_field(uv * u_pattern_scale, time, u_image_function);
    float color_index = fract(field + u_time * u_palette_speed);
    imageStore(output_image, pixel, colormap(color_index));
}
)";

AcidWarpGpt56Terra::AcidWarpGpt56Terra()
    : Art("AcidWarp-gpt56-terra") {
    useCompute();
    current_colormap_name = easel->pal.get_cmap().getTitle();
    init_shader();
    update_uniform_callback();
}

void AcidWarpGpt56Terra::init_shader() {
    ecompute()->set_compute_shader(compute_shader_base + easel->pal.get_cmap().getSource());
}

void AcidWarpGpt56Terra::update_uniform_callback() {
    ecompute()->set_uniform_callback([this](GLuint) {
        ecompute()->set_uniform_int("u_image_function", image_function);
        ecompute()->set_uniform_float("u_pattern_scale", pattern_scale);
        ecompute()->set_uniform_float("u_motion_speed", motion_speed);
        ecompute()->set_uniform_float("u_palette_speed", palette_speed);
    });
}

bool AcidWarpGpt56Terra::render(uint32_t*) {
    return true;
}

bool AcidWarpGpt56Terra::render_gui() {
    std::string new_colormap = easel->pal.get_cmap().getTitle();
    if (new_colormap != current_colormap_name) {
        current_colormap_name = new_colormap;
        init_shader();
    }

    ScrollableSliderInt("image function", &image_function, 0, 40, "%d", 1);
    ScrollableSliderFloat("pattern scale", &pattern_scale, 0.25f, 4.0f, "%.2f", 0.05f);
    ScrollableSliderFloat("motion speed", &motion_speed, 0.0f, 2.0f, "%.2f", 0.02f);
    ScrollableSliderFloat("palette speed", &palette_speed, 0.0f, 1.0f, "%.2f", 0.01f);
    return false;
}

void AcidWarpGpt56Terra::resize(int width, int height) {
    default_resize(width, height);
}
