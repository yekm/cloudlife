#include "hopalong3d.hpp"

#include "easelvertex3d.h"
#include "imgui.h"
#include "imgui_elements.h"
#include "random.h"

#include <algorithm>
#include <cmath>

namespace {

float random_range(float minimum, float maximum) {
    return minimum + static_cast<float>(LRAND() / MAXRAND) * (maximum - minimum);
}

}

void Hopalong3D::randomize_parameters() {
    a = random_range(-30.0f, 30.0f);
    b = random_range(0.2f, 1.8f);
    c = random_range(5.0f, 17.0f);
    d = random_range(0.0f, 10.0f);
    e = random_range(0.0f, 12.0f);
}

void Hopalong3D::regenerate_orbit() {
    orbit.clear();
    orbit.reserve(static_cast<size_t>(subsets) * dots_per_layer);

    double x_min = 0.0;
    double x_max = 0.0;
    double y_min = 0.0;
    double y_max = 0.0;
    bool first_point = true;

    for (int subset = 0; subset < subsets; ++subset) {
        double x = random_range(-0.0025f, 0.0025f) * (subset + 1);
        double y = random_range(-0.0025f, 0.0025f) * (subset + 1);

        // Skip the transient so the visible orbit is already settled.
        for (int i = 0; i < 100; ++i) {
            const double z = d + std::sqrt(std::abs(b * x - c));
            const double next_x = x > 0.0 ? y - z : (x < 0.0 ? y + z : y);
            y = a - x;
            x = next_x + e;
        }

        for (int point = 0; point < dots_per_layer; ++point) {
            const double z = d + std::sqrt(std::abs(b * x - c));
            const double next_x = x > 0.0 ? y - z : (x < 0.0 ? y + z : y);
            y = a - x;
            x = next_x + e;

            orbit.emplace_back(static_cast<float>(x), static_cast<float>(y));
            if (first_point) {
                x_min = x_max = x;
                y_min = y_max = y;
                first_point = false;
            } else {
                x_min = std::min(x_min, x);
                x_max = std::max(x_max, x);
                y_min = std::min(y_min, y);
                y_max = std::max(y_max, y);
            }
        }
    }

    const double width = std::max(x_max - x_min, 0.000001);
    const double height = std::max(y_max - y_min, 0.000001);
    const double scale = std::min(2.0 / width, 2.0 / height) * orbit_scale;
    const double center_x = (x_min + x_max) * 0.5;
    const double center_y = (y_min + y_max) * 0.5;

    for (auto& point : orbit) {
        point.x = static_cast<float>((point.x - center_x) * scale);
        point.y = static_cast<float>((point.y - center_y) * scale);
    }

    layer_offset = 0.0f;
    rotation = 0.0f;
    frames_since_regenerate = 0;
}

bool Hopalong3D::render(uint32_t*) {
    auto* e3d = evertex3d();
    if (orbit.empty())
        return false;

    const unsigned max_vertices = easel->vertex_buffer_maximum();
    const unsigned requested_vertices = static_cast<unsigned>(layers) * orbit.size();
    const unsigned vertices_to_draw = std::min(max_vertices, requested_vertices);
    const float total_depth = std::max(1.0f, layers * layer_depth);

    const float sine = std::sin(rotation);
    const float cosine = std::cos(rotation);

    for (unsigned index = 0; index < vertices_to_draw; ++index) {
        const unsigned orbit_index = index % orbit.size();
        const unsigned layer = index / orbit.size();
        const auto& point = orbit[orbit_index];
        const float x = point.x * cosine - point.y * sine;
        const float y = point.x * sine + point.y * cosine;
        const float z = (layer * layer_depth + layer_offset) - total_depth * 0.5f;
        const float color = static_cast<float>(orbit_index % dots_per_layer) /
            static_cast<float>(std::max(1, dots_per_layer - 1));

        e3d->drawdot(x, y, z, color);
    }

    layer_offset += travel_speed;
    if (layer_offset > layer_depth)
        layer_offset -= layer_depth;
    rotation += rotation_speed;

    if (regenerate_frames > 0 && ++frames_since_regenerate >= static_cast<unsigned>(regenerate_frames)) {
        randomize_parameters();
        regenerate_orbit();
    }

    return false;
}

bool Hopalong3D::render_gui() {
    bool changed = false;

    if (ImGui::CollapsingHeader("Hopalong 3D Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ScrollableSliderInt("Layers", &layers, 1, 16, "%d", 1);
        changed |= ScrollableSliderInt("Subsets", &subsets, 1, 16, "%d", 1);
        changed |= ScrollableSliderInt("Dots per subset", &dots_per_layer, 32, 1024, "%d", 16);
        ScrollableSliderInt("Regenerate frames", &regenerate_frames, 0, 3600, "%d (0 disables)", 30);
        ScrollableSliderFloat("Layer depth", &layer_depth, 0.05f, 1.5f, "%.2f", 0.05f);
        ScrollableSliderFloat("Travel speed", &travel_speed, 0.0f, 0.05f, "%.4f", 0.001f);
        ScrollableSliderFloat("Rotation speed", &rotation_speed, -0.02f, 0.02f, "%.4f", 0.001f);
        changed |= ScrollableSliderFloat("Orbit scale", &orbit_scale, 0.25f, 1.8f, "%.2f", 0.05f);

        ImGui::Separator();
        ImGui::Text("Barry Martin parameters");
        changed |= ScrollableSliderDouble("a", &a, -30.0, 30.0, "%.3f", 0.1);
        changed |= ScrollableSliderDouble("b", &b, 0.2, 1.8, "%.3f", 0.01);
        changed |= ScrollableSliderDouble("c", &c, 5.0, 17.0, "%.3f", 0.1);
        changed |= ScrollableSliderDouble("d", &d, 0.0, 10.0, "%.3f", 0.1);
        changed |= ScrollableSliderDouble("e", &e, 0.0, 12.0, "%.3f", 0.1);

        if (ImGui::Button("Randomize orbit")) {
            randomize_parameters();
            changed = true;
        }

        const unsigned requested = static_cast<unsigned>(layers) * subsets * dots_per_layer;
        ImGui::Text("Vertices: %u / %u", std::min(requested, easel->vertex_buffer_maximum()),
            easel->vertex_buffer_maximum());
    }

    if (changed)
        regenerate_orbit();
    return false;
}

void Hopalong3D::resize(int w, int h) {
    default_resize(w, h);
    randomize_parameters();
    regenerate_orbit();
}
