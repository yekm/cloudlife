#pragma once

#include "art.hpp"

#include <string>

class PrimeUmap3D : public Art {
public:
    PrimeUmap3D()
        : Art("Prime UMAP 3D") {
        useVertex3D();
    }

private:
    bool render_gui() override;
    void resize(int w, int h) override;
    bool render(uint32_t* p) override;

    void rebuild_embedding();

    int integer_count = 500;
    int neighbors = 15;
    int epochs = 200;
    int random_seed = 42;
    float min_distance = 0.1f;
    float repulsion_strength = 1.0f;
    float rotation_speed = 0.0015f;
    float rotation = 0.0f;

    bool parameters_dirty = false;
    bool geometry_ready = false;
    double rebuild_seconds = 0.0;
    std::string status = "Waiting for initial embedding";
};
