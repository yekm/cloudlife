#pragma once

#include "art.hpp"

#include <cstdint>
#include <vector>

class CollatzBirb3D : public Art {
public:
    CollatzBirb3D()
        : Art("Collatz Birb 3D") {
        useVertex3D();
    }

private:
    bool render_gui() override;
    void resize(int w, int h) override;
    bool render(uint32_t* p) override;

    void rebuild_chains();
    void rebuild_geometry();

    std::vector<std::vector<uint32_t>> chains;
    int maximum = 20000;
    int points_per_segment = 2;
    float segment_length = 0.035f;
    float even_angle = 3.14159265f / 13.0f;
    float odd_angle = -3.14159265f / 20.0f;
    float rotation_speed = 0.0015f;
    float rotation = 1.04719755f;
};
