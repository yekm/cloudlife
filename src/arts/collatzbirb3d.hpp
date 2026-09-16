#pragma once

// https://github.com/SpectralFlame/Collatz-birb-3D

#include "art.hpp"

#include <cstdint>
#include <vector>
#include <string>

class CollatzBirb3D : public Art {
public:
    std::string about() const override
    {
        return "Collatz Birb 3D builds branching spatial paths from integer Collatz trajectories. The header cites "
               "SpectralFlame's Collatz-birb-3D repository, which contains a Processing sketch for a "
               "three-dimensional visualization, as the reference. Cloudlife implements this idea in its native "
               "point renderer. The local source does not record a separate dated author credit for this adaptation. "
               "The underlying Collatz problem is usually associated with Lothar Collatz and 1937, but the drawing "
               "is an artistic encoding of the sequences, not a proof of their convergence.\n"
               "\n"
               "For a positive integer n, this implementation uses the accelerated rule n'=n/2 when n is even, and "
               "n'=(3*n+1)/2 when n is odd. The odd rule combines the familiar 3*n+1 step with the immediately "
               "following division by two. Seeds are considered from Maximum seed down to 2. A visited array avoids "
               "starting a new chain at a seed already encountered in an earlier chain, although common trajectory "
               "tails are still present in the stored chains. Each trajectory stops at 1 or at a length limit of "
               "4096. Integer arithmetic is uint32_t, so this bounded visualization is not an arbitrary-precision "
               "investigation of the conjecture.\n"
               "\n"
               "For drawing, a chain is traversed backward from its smallest-tail end. Each path starts at the "
               "origin with an identity orientation. At each segment, up to three successive values from that "
               "reversed chain set rotations around the local x, y, and z axes. Each parity chooses Even angle or "
               "Odd angle. These three-value windows overlap between segments, so several neighboring parity "
               "decisions jointly shape each bend. After updating the orientation, the turtle moves a fixed Segment "
               "length along its rotated negative x direction. Points per segment samples that movement uniformly, "
               "making denser paths without changing their underlying turns.\n"
               "\n"
               "Parity also adjusts three internal color channels with different weights; the weighted result "
               "selects a scalar palette position. Shared parity histories and shared tails lead to clustered paths "
               "and branching, bird-like silhouettes. They should be read as patterns in the chosen geometric "
               "encoding, rather than literal spatial positions of the integers.\n"
               "\n"
               "Maximum seed changes trajectory coverage; the two angles change curvature; Segment length changes "
               "overall spatial scale. Changing geometry controls rebuilds the chains and samples. Geometry is then "
               "frozen, and Rotation speed applies a whole-object rotation about the y axis each frame. Emission "
               "stops at the renderer's vertex capacity, so sufficiently dense settings can truncate the requested "
               "picture.\n"
               "\n"
               "References and further reading:\n"
               "https://github.com/SpectralFlame/Collatz-birb-3D\n"
               "https://en.wikipedia.org/wiki/Collatz_conjecture\n"
               "https://en.wikipedia.org/wiki/Turtle_graphics";
    }

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
