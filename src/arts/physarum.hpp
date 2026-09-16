#pragma once

// https://github.com/nicoptere/physarum
// https://github.com/fogleman/physarum

#include "art.hpp"

#include <array>
#include <random>
#include <vector>
#include <string>

class Physarum : public Art {
public:
    std::string about() const override
    {
        return "History and provenance\n"
               "The header cites the nicoptere/physarum and fogleman/physarum repositories as references. They "
               "provide JavaScript/WebGL and Go examples of slime-mold-inspired simulation. The local source "
               "does not record an author or creation date for the Cloudlife implementation or establish that it "
               "is a direct copy of either project. The biological inspiration is Physarum polycephalum, whose "
               "trail-following behavior motivates this simplified agent model.\n"
               "\n"
               "Algorithm\n"
               "Each species has particles with a position and heading, a trail grid, and its own movement and "
               "sensing parameters. Initialization scatters particles and headings randomly, fills trails with "
               "small random values, and normally makes species attracted to their own trails and repelled by "
               "other species. Before moving particles, the simulation constructs one sensed field per species "
               "as a weighted sum of all trail grids. The attraction matrix supplies those weights, with "
               "positive values attracting and negative values repelling.\n"
               "\n"
               "Each particle samples that field ahead and at two directions offset by sensor angle, all at the "
               "configured sensor distance. If the forward signal is weaker than both side signals, it chooses a "
               "random turn direction. Otherwise it turns toward the stronger side, or keeps its direction when "
               "side signals tie. It moves by step distance and adds deposition amount to its own species' trail "
               "at the destination. Grid sampling and deposition wrap periodically at the image boundaries.\n"
               "\n"
               "After all particles move, each species' grid is blurred by horizontal and vertical three-sample "
               "averages, equivalent to a 3 by 3 box filter, and multiplied by its decay factor. Thus deposited "
               "material diffuses and gradually disappears. Rendering maps trail intensity through a clamped "
               "square root, adds fixed species colors, and clips the summed RGB values. CPU cost is controlled "
               "by limiting the simulation's longer dimension to roughly 512 cells and scaling it to the "
               "display.\n"
               "\n"
               "Controls set particles per species, species count, simulation steps per frame, and the selected "
               "species' sensor angle/distance, turn angle, step distance, deposition, decay, and attraction "
               "from each species. Population changes and shuffle reinitialize the simulation. Networks emerge "
               "from local feedback; this is an artistic model rather than a complete biological simulation.\n"
               "\n"
               "References\n"
               "https://github.com/nicoptere/physarum\n"
               "https://github.com/fogleman/physarum\n"
               "https://en.wikipedia.org/wiki/Physarum_polycephalum\n"
               "https://en.wikipedia.org/wiki/Chemotaxis\n"
               "https://en.wikipedia.org/wiki/Agent-based_model";
    }

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
