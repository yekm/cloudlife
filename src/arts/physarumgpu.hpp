#pragma once

#ifndef __APPLE__

#include "art.hpp"

#include <glad/glad.h>

class PhysarumGPU : public Art {
public:
    PhysarumGPU();
    ~PhysarumGPU() override;

private:
    bool render(uint32_t* pixels) override;
    bool render_gui() override;
    void resize(int width, int height) override;
    void shuffle() override;

    void create_resources();
    void destroy_resources();
    void reset_simulation();
    GLuint compile_compute_program(const char* source) const;

    struct Config {
        float sensor_angle;
        float sensor_distance;
        float rotation_angle;
        float step_distance;
        float deposition_amount;
        float decay_factor;
    };

    GLuint m_particle_program = 0;
    GLuint m_trail_program = 0;
    GLuint m_composite_program = 0;
    GLuint m_particle_buffer = 0;
    GLuint m_trails[2] = {0, 0};
    GLuint m_deposits = 0;
    int m_current_trail = 0;
    int m_width = 0;
    int m_height = 0;
    int m_particles_per_species = 20000;
    int m_steps_per_frame = 1;
    int m_species_count = 3;
    int m_selected_species = 0;
    Config m_configs[5] = {};
    float m_attraction[25] = {};
};

#endif
