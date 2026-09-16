#pragma once

// https://github.com/dghost/hopalong-vr

#include "art.hpp"

#include <glm/vec2.hpp>
#include <vector>
#include <string>

class Hopalong3D : public Art {
public:
    std::string about() const override
    {
        return "Hopalong 3D turns a planar nonlinear orbit into a moving stack of point-cloud layers. Its header "
               "cites dghost's HopalongVR project as its reference. That project's README describes a WebVR port of "
               "Iacopo Sassarini's Barry Martin Hopalong Attractor Visualizer. Barry Martin is also credited for the "
               "original Hopalong construction in the historical comments of Cloudlife's separate two-dimensional "
               "Hopalong art, which refer to Scientific American, September 1986. This implementation adapts the "
               "orbit-and-layer idea to Cloudlife's native 3D renderer; the local source does not give a separate "
               "dated authorship credit for this adaptation.\n"
               "\n"
               "For each subset, the algorithm starts (x,y) at a small random displacement near the origin. Define q "
               "= d + sqrt(abs(b*x-c)). One iteration computes x' = y-sign(x)*q+e and y' = a-x, with the square-root "
               "contribution omitted when x is exactly zero. Parameters a, b, and c shape the usual Hopalong "
               "feedback; d adds a signed displacement, and e shifts the new x coordinate. A hundred transient "
               "iterations are skipped before collecting visible points, so the displayed orbit begins after the "
               "initial settling period. Each subset contributes the selected number of dots, exposing how nearby "
               "initial points explore the recurrence.\n"
               "\n"
               "The collected points are centered on their combined bounding box and uniformly scaled to fit the "
               "display, multiplied by Orbit scale. The same planar cloud is then repeated at evenly spaced depths. "
               "Each frame rotates its x/y coordinates and advances a shared depth offset, which wraps after one "
               "layer spacing. The tunnel-like motion is created by arranging and moving copies of a two-dimensional "
               "map; the recurrence itself does not acquire a third state variable.\n"
               "\n"
               "Layers, Subsets, and Dots per subset determine the requested point count: their product. Rendering "
               "stops at the renderer's vertex capacity, shown in the settings. Layer depth controls separation, "
               "Travel speed controls forward motion, and Rotation speed controls planar turning. These motion "
               "controls preserve the sampled orbit. Changing the orbit parameters, point counts, or Orbit scale "
               "resamples it. Randomize orbit chooses new parameter values; Regenerate frames does so automatically, "
               "with zero disabling automatic regeneration. Resampling also resets the motion counters.\n"
               "\n"
               "References and further reading:\n"
               "https://github.com/dghost/hopalong-vr\n"
               "https://iacopoapps.appspot.com/hopalongwebgl/\n"
               "https://en.wikipedia.org/wiki/Chaos_theory\n"
               "https://en.wikipedia.org/wiki/Attractor";
    }

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
