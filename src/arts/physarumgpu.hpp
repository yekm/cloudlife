#pragma once

#ifndef __APPLE__

#include "art.hpp"

#include <glad/glad.h>
#include <string>

class PhysarumGPU : public Art {
public:
    std::string about() const override
    {
        return "History and provenance\n"
               "This is Cloudlife's GPU implementation of the slime-mold-inspired particle-and-trail model also "
               "used by its CPU Physarum art. The CPU header references nicoptere/physarum and "
               "fogleman/physarum; the GPU source itself does not give a separate upstream credit, author, or "
               "creation date. Those projects are useful background, not confirmed authorship of this "
               "compute-shader implementation. The organism Physarum polycephalum supplies the biological "
               "inspiration for trail-following agents.\n"
               "\n"
               "Algorithm\n"
               "Particles store position, heading, and species in a GPU buffer. Each species has a "
               "floating-point trail layer and separate sensing and movement parameters. One compute invocation "
               "updates each particle, sampling the trail mixture forward, left, and right. The attraction "
               "matrix weights trails from every species: positive weights attract and negative weights repel. "
               "If both side signals exceed the forward one, an id-and-frame hash selects a turn direction; "
               "otherwise the particle turns toward the stronger side. It advances by step distance and wraps "
               "its position around the image edges.\n"
               "\n"
               "The particle pass atomically increments an integer deposit count at the destination cell, "
               "allowing concurrent arrivals without lost increments. A second pass averages each old trail "
               "layer over its periodic 3 by 3 neighborhood, multiplies by the species' decay factor, and adds "
               "deposit count times deposition amount. It clears those counts for the next step and writes a "
               "separate trail texture, then swaps source and destination. Unlike the CPU version, freshly "
               "deposited material is added after that step's blur. Initial GPU trails are zero. A final shader "
               "adds fixed species colors using clamped square-root trail intensities and writes the image. The "
               "GPU grid uses the display dimensions.\n"
               "\n"
               "Controls choose particles per species, species count, and steps per frame, plus sensor angle and "
               "distance, turn angle, movement distance, deposition, decay, and each species' attraction "
               "weights. Population changes and shuffle reset particles and parameters. The network patterns "
               "arise from repeated local sensing, deposition, diffusion, and forgetting. This art requires "
               "compute-shader support and is unavailable on the macOS OpenGL backend.\n"
               "\n"
               "References\n"
               "https://github.com/nicoptere/physarum\n"
               "https://github.com/fogleman/physarum\n"
               "https://en.wikipedia.org/wiki/Physarum_polycephalum\n"
               "https://en.wikipedia.org/wiki/Chemotaxis\n"
               "https://en.wikipedia.org/wiki/Agent-based_model";
    }

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
