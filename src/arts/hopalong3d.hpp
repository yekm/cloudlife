#pragma once

#include "art.hpp"

#include <glm/vec2.hpp>
#include <vector>

class Hopalong3D : public Art {
public:
    Hopalong3D()
        : Art("Hopalong 3D") {
        useVertex3D();
    }

private:
    bool render_gui() override;
    void resize(int w, int h) override;
    bool render(uint32_t* p) override;

    void regenerate_orbit();
    void randomize_parameters();

    std::vector<glm::vec2> orbit;

    int layers = 8;
    int subsets = 6;
    int dots_per_layer = 300;
    int regenerate_frames = 420;

    float layer_depth = 0.55f;
    float travel_speed = 0.006f;
    float rotation_speed = 0.0015f;
    float orbit_scale = 1.15f;

    double a = 0.0;
    double b = 1.0;
    double c = 10.0;
    double d = 2.0;
    double e = 6.0;

    float layer_offset = 0.0f;
    float rotation = 0.0f;
    unsigned frames_since_regenerate = 0;
};
