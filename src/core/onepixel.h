#include "art.hpp"
#include "imgui.h"
#include <vector>

class OnePixel : public Art {
public:
    std::string about() const override
    {
        return "History and sources\n"
               "A small Cloudlife rendering benchmark defined in src/core/onepixel.h. "
               "The source contains no separate author or creation date and cites no external art.\n\n"
               "Algorithm\n"
               "Every frame submits a single point at clip coordinates (0.5, 0.5) through "
               "EaselVertex, placing it three quarters across and up the framebuffer. There is no "
               "simulation or random sampling. The shared renderer batches the point into a vertex "
               "buffer and draws it using OpenGL's point primitive. Repeated submissions accumulate "
               "in the shared renderer's bounded buffer, overlapping at the same position. The "
               "shader passes coordinates directly to clip space; the renderer advances the palette "
               "index on each submission. The Art base class "
               "still runs its normal begin, render, and frame-counting steps.\n\n"
               "Use this art as a minimal drawing workload when comparing rendering overhead. "
               "Observed frame time also includes the GUI, buffer handling, window system, and "
               "frame synchronization; it does not isolate the cost of one GPU point.\n\n"
               "Controls\n"
               "There are no art-specific parameters. Shared vertex appearance controls determine "
               "the point's appearance. Shuffle has no art-specific effect.\n\n"
               "Further reading\n"
               "Benchmarking\n"
               "https://en.wikipedia.org/wiki/Benchmark_(computing)\n"
               "Graphics primitives\n"
               "https://en.wikipedia.org/wiki/Geometric_primitive";
    }

    OnePixel()
        : Art("One pixel benchmark") {
            useVertex();
            //use_pixel_buffer = true;
            //pixel_buffer_maximum = 1024*512;
        }
private:
    //virtual bool render_gui() override {};
    //virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override {
        //drawdot(p, w/2, h/2, 0x00ffffff);
        drawdot(.5, .5);
        return true;
    }
};

class Nothing : public Art {
public:
    std::string about() const override
    {
        return "History and sources\n"
               "A Cloudlife utility defined in src/core/onepixel.h alongside the single-point "
               "benchmark. The source records no separate author, creation date, or upstream art.\n\n"
               "Algorithm\n"
               "The art selects EaselVertex but submits no vertices: its render function simply "
               "returns false. The Art base class still executes the normal frame lifecycle and "
               "increments the frame counter. The renderer has no point batch to draw. The visible "
               "canvas therefore consists of the application's framebuffer clear color, plus the "
               "GUI when it is shown. There is no hidden particle system or simulation state.\n\n"
               "This provides an empty-workload baseline for comparing Cloudlife's overhead with "
               "OnePixel or more complex arts. Measurements still include event processing, "
               "GUI rendering, window updates, and frame synchronization. Compare under the same "
               "window size and synchronization settings to make the baseline useful.\n\n"
               "Controls\n"
               "There are no art-specific controls. The application's clear color changes the "
               "background; Shuffle has no art-specific effect.\n\n"
               "Further reading\n"
               "Benchmarking\n"
               "https://en.wikipedia.org/wiki/Benchmark_(computing)\n"
               "Framebuffers\n"
               "https://en.wikipedia.org/wiki/Framebuffer";
    }

    Nothing()
        : Art("Nothing") {
            printf(__func__); printf("\n"); fflush(stdout);
            useVertex();
        }
private:
    virtual bool render(uint32_t *p) override {
        return false;
    }
};
