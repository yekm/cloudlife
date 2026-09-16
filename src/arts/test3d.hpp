#pragma once

#include "art.hpp"
#include <string>

class Test3D : public Art {
public:
    std::string about() const override
    {
        return "History and sources\n"
               "A Cloudlife demonstration of the EaselVertex3D renderer, introduced in a commit by "
               "Pavel V on 23 February 2026 alongside the GLM-based 3D backend. The source does not "
               "attribute this art to an external screensaver.\n\n"
               "Algorithm\n"
               "Each frame independently samples x, y, and z with balance_rand(4), giving a uniform "
               "point cloud in the cube [-2, 2) on each axis. This is a cube distribution, even though "
               "the colors have spherical symmetry. For each point compute "
               "r = sqrt(x*x + y*y + z*z) and submit r / 4 as its palette coordinate. "
               "With a colormap enabled, distance from the origin determines color; the shared "
               "renderer can also use a uniform color.\n\n"
               "The renderer batches positions and color coordinates into vertex buffers. Model, "
               "view, and perspective projection matrices transform each point to the screen. "
               "Additive blending makes overlapping points brighter. New samples are generated "
               "every frame; there is no particle motion, attractor, or persistent 3D dataset in "
               "this art.\n\n"
               "Controls\n"
               "Points per frame changes the sampling and drawing workload. The shared camera "
               "controls change position and projection; point size, opacity, and palette change "
               "appearance. This small art helps exercise the 3D rendering path.\n\n"
               "Further reading\n"
               "Point clouds\n"
               "https://en.wikipedia.org/wiki/Point_cloud\n"
               "3D projection\n"
               "https://en.wikipedia.org/wiki/3D_projection";
    }

    Test3D() : Art("Test 3D") {
        useVertex3D();
    }
    
private:
    bool render_gui() override;
    bool render(uint32_t *p) override;

    int points_per_frame = 1000;
};
