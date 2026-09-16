#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"
#include <string>

/* costs ~6% speed */
#define dither_when_mapped 1

class RDbomb : public Art {
public:
    std::string about() const override
    {
        return "RDbomb is Scott Draves's 1997 reaction-diffusion texture generator, derived from his Bomb "
               "program and included in XScreenSaver. The code explicitly cites John E. Pearson's July 1993 "
               "paper, Complex Patterns in a Simple System; a copy is preserved in this repository's "
               "archive. Pavel Vasilyev's Dear ImGui port is dated June 2023. The visual patterns come from "
               "interacting fields, rather than from a precomputed picture.\n\n"
               "Two bounded, 16-bit fields evolve on a rectangular grid with periodic boundaries: material "
               "crossing one side re-enters from the opposite side. Each update first mixes a cell with its "
               "four axial neighbors. The two fields use different weighted averages, producing different "
               "effective diffusion rates. Diffusion mode selects among three stencils; one also weights "
               "horizontal neighbors differently from vertical neighbors, introducing directional "
               "preference.\n\n"
               "A local nonlinear reaction then transfers material between the fields. In normalized "
               "notation its central interaction is U*V*V: it consumes U and produces V. A feed term "
               "replenishes U toward one, and a removal term drains V. Mode 0 approximately adds "
               "4*((28/1024)*(1-U)-U*V*V) to U and 4*(U*V*V-(80/1024)*V) to V after diffusion. Other "
               "reaction modes change the coefficients or relative update multipliers. The actual "
               "implementation evaluates these expressions with integer shifts and clamps the results to "
               "the available range.\n\n"
               "The initial grid is close to a uniform equilibrium. Init type perturbs V with a central "
               "square, four separated seed points, or sparse random points within the seed region; radius "
               "controls the extent or separation of that perturbation. Local growth, depletion, and "
               "spreading can then generate spots, split structures, traveling fronts, and labyrinth-like "
               "textures.\n\n"
               "Iterations controls simulation updates per rendered frame. Width and height set the "
               "simulation resolution, influencing both cost and the space available for pattern formation. "
               "Reaction and diffusion choose the rule variants, while ncolors sets the range used when "
               "mapping U to the current palette. The art's shuffle operation randomizes reaction and "
               "diffusion. The tutorial below explains the related Gray-Scott model; its floating-point "
               "parameters are background reading rather than direct substitutes for these integer mode "
               "selectors.\n\n"
               "Further reading:\n"
               "https://arxiv.org/abs/patt-sol/9304003\n"
               "https://www.karlsims.com/rd.html\n"
               "https://en.wikipedia.org/wiki/Reaction%E2%80%93diffusion_system";
    }

    RDbomb()
        : Art("reaction/diffusion textures") {
            shuffle_period = 42;
        }
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;
    virtual void shuffle() override;

    void pixack_init();
    void pixack_frame();
    void random_colors();
    void rd_init();

#if dither_when_mapped
    unsigned char *mc;
#endif
    int ncolors = 65535;
    int iterations = 1;
    int init_type = 0;

    int mapped;
    int pdepth;

    std::vector<unsigned short> r1, r2, r1b, r2b;
    int width = 512, height = width, npix;
    int radius = 8;
    int reaction = 1;
    int diffusion = 1;

    char *pd;
    int array_width, array_height;

    double array_x, array_y;
    double array_dx, array_dy;
};

