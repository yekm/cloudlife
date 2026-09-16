#include "art.hpp"
#include "imgui.h"
#include <memory>
#include <vector>

#include "settings.hpp"
#include "easelvertex.h"
#include "strange.hpp"
#include <string>

class Attractor : public Art {
public:
    std::string about() const override
    {
        return "Strange attractors currently displays the Gumowski-Mira map: a two-dimensional nonlinear recurrence "
               "whose repeated points can form elaborate symmetric-looking patterns. The implementation explicitly "
               "references HoloViz's Visualizing Attractors gallery in strange.hpp. That gallery attributes this map "
               "to I. Gumowski and C. Mira, and its example code and parameters to Jason Rampe and Lazaro Alonso. "
               "Cloudlife uses the same equations and its initial parameter example. The local files do not provide "
               "a separate date or author credit for the Cloudlife adaptation. Despite the broad art title and other "
               "map classes present in strange.hpp, this art instantiates only AGumowskiMira; it is not the archived "
               "xscreensaver strange.c algorithm.\n"
               "\n"
               "Define G(x,mu) = mu*x + 2*(1-mu)*x*x/(1+x*x). Starting from (x,y), a single update first computes xn "
               "= y + a*(1-b*y*y)*y + G(x,mu), then yn = -x + G(xn,mu). Both returned coordinates become the state "
               "for the next update. In this GUI, c supplies mu. The rational term bends the x feedback, while the "
               "cubic y term contributes amplitude-dependent behavior. Repeated coupling can create filaments and "
               "folded structures, but arbitrary parameter values can also yield periodic behavior or escaping "
               "trajectories; an interesting picture is not guaranteed by every slider setting.\n"
               "\n"
               "The default initial point is (0,1), with a=0.008, b=0.05, and c=-0.496. A and b control the "
               "nonlinear y contribution; c changes G. E, i and f, j specify the initial x and y coordinates used "
               "when the map resets. These controls allow comparison of both equation changes and different starting "
               "conditions. The d and inc fields belong to the shared attractor interface and are not used by this "
               "particular map, although changing them still clears and resets the drawing.\n"
               "\n"
               "Mul scales the plotted coordinates and can reflect the picture if negative. Its change also restarts "
               "the orbit. The renderer's points-per-frame target determines how many successive states are emitted "
               "per frame; samples continue from the preceding frame's state, accumulating detail over time. Color "
               "follows point emission rather than representing a mathematical scalar such as a Lyapunov exponent. "
               "Changing a parameter clears the image, restores the selected initial point, and starts a new orbit.\n"
               "\n"
               "References and further reading:\n"
               "https://examples.holoviz.org/gallery/attractors/attractors.html\n"
               "https://en.wikipedia.org/wiki/Attractor\n"
               "https://en.wikipedia.org/wiki/Chaos_theory";
    }

    Attractor()
        : Art("strange attractors") {
            useVertex();
            attractor = std::make_unique<AGumowskiMira>();
		}

private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    void init();

	long unsigned int count = 0;

    double mul = 1;

    std::unique_ptr<StrangeAttractor<>> attractor;
};
