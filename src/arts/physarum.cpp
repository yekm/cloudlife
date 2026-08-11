#include "physarum.hpp"

#include "imgui_elements.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float PI = 3.14159265358979323846f;
constexpr std::array<uint32_t, 5> colors = {
    0xff3156fau, 0xff1fbfffu, 0xff46f1ffu, 0xff19e3abu, 0xff81c400u,
};
}

Physarum::Physarum()
    : Art("Physarum"), m_random(std::random_device{}()) {
    usePlane();
}

float Physarum::random(float minimum, float maximum) {
    return minimum + m_unit_distribution(m_random) * (maximum - minimum);
}

unsigned Physarum::index(float x, float y) const {
    int xi = static_cast<int>(x);
    int yi = static_cast<int>(y);
    xi %= m_width;
    yi %= m_height;
    if (xi < 0) xi += m_width;
    if (yi < 0) yi += m_height;
    return static_cast<unsigned>(yi * m_width + xi);
}

void Physarum::resize(int width, int height) {
    default_resize(width, height);

    const int longest_side = std::max(width, height);
    const float scale = longest_side > 512 ? 512.0f / longest_side : 1.0f;
    m_width = std::max(64, static_cast<int>(width * scale));
    m_height = std::max(64, static_cast<int>(height * scale));
    reset_simulation();
}

void Physarum::shuffle() {
    if (m_width > 0 && m_height > 0) reset_simulation();
}

void Physarum::reset_simulation() {
    m_configs.clear();
    m_grids.assign(m_species_count, {});
    m_particles.clear();
    m_particles.reserve(static_cast<size_t>(m_species_count) * m_particles_per_species);
    m_attraction.assign(m_species_count, std::vector<float>(m_species_count));

    const size_t grid_size = static_cast<size_t>(m_width) * m_height;
    for (int species = 0; species < m_species_count; ++species) {
        m_configs.push_back({
            random(20.0f, 100.0f) * PI / 180.0f,
            random(5.0f, 30.0f),
            random(20.0f, 100.0f) * PI / 180.0f,
            random(0.6f, 1.8f),
            5.0f,
            0.92f,
        });
        m_grids[species].data.resize(grid_size);
        m_grids[species].sensed.resize(grid_size);
        m_grids[species].scratch.resize(grid_size);
        for (float& value : m_grids[species].data) value = random(0.0f, 1.0f);

        for (int i = 0; i < m_particles_per_species; ++i) {
            m_particles.push_back({
                random(0.0f, static_cast<float>(m_width)),
                random(0.0f, static_cast<float>(m_height)),
                random(0.0f, 2.0f * PI),
                static_cast<unsigned>(species),
            });
        }
    }

    for (int target = 0; target < m_species_count; ++target) {
        for (int source = 0; source < m_species_count; ++source) {
            m_attraction[target][source] = target == source ? random(0.75f, 1.25f)
                                                              : random(-1.25f, -0.75f);
        }
    }
}

void Physarum::blur_and_decay(Grid& grid, float decay) {
    for (int y = 0; y < m_height; ++y) {
        for (int x = 0; x < m_width; ++x) {
            grid.scratch[y * m_width + x] =
                (grid.data[y * m_width + (x + m_width - 1) % m_width] +
                 grid.data[y * m_width + x] +
                 grid.data[y * m_width + (x + 1) % m_width]) / 3.0f;
        }
    }
    for (int y = 0; y < m_height; ++y) {
        const int previous = (y + m_height - 1) % m_height;
        const int next = (y + 1) % m_height;
        for (int x = 0; x < m_width; ++x) {
            grid.data[y * m_width + x] = decay *
                (grid.scratch[previous * m_width + x] + grid.scratch[y * m_width + x] +
                 grid.scratch[next * m_width + x]) / 3.0f;
        }
    }
}

void Physarum::step() {
    const size_t grid_size = static_cast<size_t>(m_width) * m_height;
    for (int target = 0; target < m_species_count; ++target) {
        std::fill(m_grids[target].sensed.begin(), m_grids[target].sensed.end(), 0.0f);
        for (int source = 0; source < m_species_count; ++source) {
            const float attraction = m_attraction[target][source];
            for (size_t i = 0; i < grid_size; ++i) {
                m_grids[target].sensed[i] += m_grids[source].data[i] * attraction;
            }
        }
    }

    for (Particle& particle : m_particles) {
        const Config& config = m_configs[particle.species];
        const Grid& grid = m_grids[particle.species];
        const auto sense = [&](float angle) {
            return grid.sensed[index(particle.x + std::cos(angle) * config.sensor_distance,
                                     particle.y + std::sin(angle) * config.sensor_distance)];
        };
        const float center = sense(particle.angle);
        const float left = sense(particle.angle - config.sensor_angle);
        const float right = sense(particle.angle + config.sensor_angle);
        if (center < left && center < right) {
            particle.angle += random(0.0f, 1.0f) < 0.5f ? -config.rotation_angle : config.rotation_angle;
        } else if (left > right) {
            particle.angle -= config.rotation_angle;
        } else if (right > left) {
            particle.angle += config.rotation_angle;
        }
        particle.x += std::cos(particle.angle) * config.step_distance;
        particle.y += std::sin(particle.angle) * config.step_distance;
        m_grids[particle.species].data[index(particle.x, particle.y)] += config.deposition_amount;
    }

    for (int species = 0; species < m_species_count; ++species) {
        blur_and_decay(m_grids[species], m_configs[species].decay_factor);
    }
}

bool Physarum::render(uint32_t*) {
    for (int i = 0; i < m_steps_per_frame; ++i) step();

    for (int y = 0; y < easel->h; ++y) {
        const int simulation_y = y * m_height / easel->h;
        for (int x = 0; x < easel->w; ++x) {
            const int simulation_x = x * m_width / easel->w;
            const unsigned sample = static_cast<unsigned>(simulation_y * m_width + simulation_x);
            float red = 0.0f;
            float green = 0.0f;
            float blue = 0.0f;
            for (int species = 0; species < m_species_count; ++species) {
                const float intensity = std::sqrt(std::min(1.0f, m_grids[species].data[sample] / 40.0f));
                const uint32_t color = colors[species % colors.size()];
                red += ((color >> 16) & 0xff) * intensity;
                green += ((color >> 8) & 0xff) * intensity;
                blue += (color & 0xff) * intensity;
            }
            drawdot(x, y, 0xff000000u | (static_cast<uint32_t>(std::min(red, 255.0f)) << 16) |
                    (static_cast<uint32_t>(std::min(green, 255.0f)) << 8) |
                    static_cast<uint32_t>(std::min(blue, 255.0f)));
        }
    }
    return false;
}

bool Physarum::render_gui() {
    bool reset = false;
    reset |= ScrollableSliderInt("particles / species", &m_particles_per_species, 1000, 100000, "%d", 1000);
    ScrollableSliderInt("steps / frame", &m_steps_per_frame, 1, 8, "%d", 1);
    reset |= ScrollableSliderInt("species", &m_species_count, 1, 5, "%d", 1);
    if (reset) {
        m_selected_species = std::min(m_selected_species, m_species_count - 1);
        reset_simulation();
    }

    if (m_configs.empty()) return false;

    ImGui::Separator();
    ScrollableSliderInt("edit species", &m_selected_species, 0, m_species_count - 1, "%d", 1);
    Config& config = m_configs[m_selected_species];
    float sensor_angle = config.sensor_angle * 180.0f / PI;
    float rotation_angle = config.rotation_angle * 180.0f / PI;
    if (ScrollableSliderFloat("sensor angle", &sensor_angle, 0.0f, 120.0f, "%.1f deg", 1.0f)) {
        config.sensor_angle = sensor_angle * PI / 180.0f;
    }
    ScrollableSliderFloat("sensor distance", &config.sensor_distance, 0.0f, 64.0f, "%.1f", 1.0f);
    if (ScrollableSliderFloat("rotation angle", &rotation_angle, 0.0f, 120.0f, "%.1f deg", 1.0f)) {
        config.rotation_angle = rotation_angle * PI / 180.0f;
    }
    ScrollableSliderFloat("step distance", &config.step_distance, 0.2f, 2.0f, "%.2f", 0.1f);
    ScrollableSliderFloat("deposition amount", &config.deposition_amount, 0.0f, 10.0f, "%.2f", 0.5f);
    ScrollableSliderFloat("decay factor", &config.decay_factor, 0.0f, 1.0f, "%.3f", 0.01f);

    ImGui::Separator();
    ImGui::Text("Attraction for species %d", m_selected_species + 1);
    for (int source = 0; source < m_species_count; ++source) {
        ImGui::PushID(source);
        ScrollableSliderFloat("from species", &m_attraction[m_selected_species][source], -2.0f, 2.0f,
                              "%.2f", 0.05f);
        ImGui::SameLine();
        ImGui::Text("%d", source + 1);
        ImGui::PopID();
    }
    return false;
}
