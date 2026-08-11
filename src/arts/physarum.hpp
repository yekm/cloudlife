#pragma once

#include "art.hpp"

#include <array>
#include <random>
#include <vector>

class Physarum : public Art {
public:
    Physarum();

private:
    struct Config {
        float sensor_angle;
        float sensor_distance;
        float rotation_angle;
        float step_distance;
        float deposition_amount;
        float decay_factor;
    };

    struct Particle {
        float x;
        float y;
        float angle;
        unsigned species;
    };

    struct Grid {
        std::vector<float> data;
        std::vector<float> sensed;
        std::vector<float> scratch;
    };

    bool render(uint32_t* pixels) override;
    bool render_gui() override;
    void resize(int width, int height) override;
    void shuffle() override;

    void reset_simulation();
    void step();
    void blur_and_decay(Grid& grid, float decay);
    unsigned index(float x, float y) const;
    float random(float minimum, float maximum);

    int m_width = 0;
    int m_height = 0;
    int m_particles_per_species = 20000;
    int m_steps_per_frame = 1;
    int m_species_count = 3;
    int m_selected_species = 0;
    std::vector<Config> m_configs;
    std::vector<Grid> m_grids;
    std::vector<Particle> m_particles;
    std::vector<std::vector<float>> m_attraction;
    std::mt19937 m_random;
    std::uniform_real_distribution<float> m_unit_distribution{0.0f, 1.0f};
};
