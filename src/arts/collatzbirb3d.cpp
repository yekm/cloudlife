#include "collatzbirb3d.hpp"

#include "easelvertex3d.h"
#include "imgui.h"
#include "imgui_elements.h"

#include <algorithm>
#include <cmath>

namespace {

uint32_t collatz_next(uint32_t value) {
    if ((value & 1u) == 0)
        return value / 2;
    return (3 * value + 1) / 2;
}

}

void CollatzBirb3D::rebuild_chains() {
    chains.clear();
    std::vector<uint8_t> visited(static_cast<size_t>(maximum) + 1, 0);

    for (uint32_t seed = static_cast<uint32_t>(maximum); seed >= 2; --seed) {
        if (visited[seed])
            continue;

        std::vector<uint32_t> chain;
        uint32_t value = seed;
        while (value != 1 && chain.size() < 4096) {
            chain.push_back(value);
            if (value <= static_cast<uint32_t>(maximum))
                visited[value] = 1;
            value = collatz_next(value);
        }
        if (chain.size() > 1)
            chains.push_back(std::move(chain));
    }
}

bool CollatzBirb3D::render(uint32_t*) {
    auto* e3d = evertex3d();
    const unsigned capacity = easel->vertex_buffer_maximum();
    unsigned emitted = 0;
    const glm::mat4 global_rotation = glm::rotate(glm::mat4(1.0f), rotation,
        glm::vec3(0.0f, 1.0f, 0.0f));

    for (const auto& source_chain : chains) {
        if (emitted >= capacity)
            break;

        glm::mat4 orientation = global_rotation;
        glm::vec3 position(0.0f);
        float red = 92.0f;
        float green = 92.0f;
        float blue = 92.0f;

        for (size_t i = 0; i < source_chain.size() && emitted < capacity; ++i) {
            for (size_t j = 0; j < 3 && i + j < source_chain.size(); ++j) {
                const uint32_t value = source_chain[source_chain.size() - 1 - i - j];
                const bool even = (value & 1u) == 0;
                const float angle = even ? even_angle : odd_angle;
                const glm::vec3 axis = j == 0 ? glm::vec3(1, 0, 0) :
                    (j == 1 ? glm::vec3(0, 1, 0) : glm::vec3(0, 0, 1));
                orientation = glm::rotate(orientation, angle, axis);

                float* channel = j == 0 ? &red : (j == 1 ? &green : &blue);
                *channel += even ? (j == 0 ? 40.0f : (j == 1 ? 4.0f : 2.0f))
                                  : -(j == 0 ? 40.0f : (j == 1 ? 4.0f : 2.0f));
                *channel = std::clamp(*channel, j == 1 ? 10.0f : 96.0f, 255.0f);
            }

            const glm::vec3 direction = glm::vec3(orientation *
                glm::vec4(-segment_length, 0.0f, 0.0f, 0.0f));
            const float color = std::clamp((red * 0.5f + green * 0.25f + blue * 0.25f) /
                255.0f, 0.0f, 1.0f);

            for (int point = 1; point <= points_per_segment && emitted < capacity; ++point) {
                const float fraction = static_cast<float>(point) / points_per_segment;
                const glm::vec3 sample = position + direction * fraction;
                e3d->drawdot(sample.x, sample.y, sample.z, color);
                ++emitted;
            }
            position += direction;
        }
    }

    rotation += rotation_speed;
    return false;
}

bool CollatzBirb3D::render_gui() {
    bool changed = false;
    if (ImGui::CollapsingHeader("Collatz Birb 3D Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed |= ScrollableSliderInt("Maximum seed", &maximum, 100, 100000, "%d", 100);
        changed |= ScrollableSliderInt("Points per segment", &points_per_segment, 1, 6, "%d", 1);
        ScrollableSliderFloat("Segment length", &segment_length, 0.005f, 0.1f, "%.3f", 0.005f);
        ScrollableSliderFloat("Even angle", &even_angle, -0.5f, 0.5f, "%.4f", 0.01f);
        ScrollableSliderFloat("Odd angle", &odd_angle, -0.5f, 0.5f, "%.4f", 0.01f);
        ScrollableSliderFloat("Rotation speed", &rotation_speed, -0.02f, 0.02f, "%.4f", 0.001f);
        ImGui::Text("Chains: %zu", chains.size());
    }
    if (changed)
        rebuild_chains();
    return false;
}

void CollatzBirb3D::resize(int w, int h) {
    default_resize(w, h);
    rebuild_chains();
}
