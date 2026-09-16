#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"
#include <string>


struct thornbirdstruct {
	int         maxx;
	int         maxy;	/* max of the screen */
	double      a;
	double      b;
	double      c;
	double      d;
	double      e;
	double      i;
	double      j;		/* thornbird parameters */
    struct {
	  double  f1 = 500;
	  double  f2 = 200;
	}           liss;
    struct {
	  double  theta;
	  double  dtheta;
	  double  phi;
	  double  dphi;
	}           tumble;
    int         inc;
	int         count;
};


class Thornbird : public Art {
public:
    std::string about() const override
    {
        return "Thornbird animates the Bird in a Thornbush fractal by gradually changing its recurrence parameters "
               "and its viewing direction. The code carries Tim Auckland's 1996 copyright. Its revision history says "
               "it was adapted from his discrete.c on 31 July 1997, with three-dimensional tumbling added by "
               "Auckland on 4 June 1999. The original xscreensaver source is preserved in "
               "arχiv/xscreensaver/hacks/thornbird.c; README.md repeats the original credit. The Cloudlife source "
               "credits Pavel Vasilyev's Dear ImGui port on 11 July 2023.\n"
               "\n"
               "The evolving state has three coordinates, (i,j,b). For each plotted point the implementation saves "
               "the old i and j, then computes j' = i, i' = (1-c)*cos(pi*a*j) + c*b, and b' = j. Thus two "
               "coordinates act as delayed memories of the nonlinear cosine term. Repeated iteration of this "
               "inexpensive rule creates intricate point distributions; changing the feedback weights and cosine "
               "frequency changes the shape. The visible picture accumulates samples of this evolving orbit rather "
               "than outlining an analytically calculated boundary.\n"
               "\n"
               "For drawing-batch index t, the parameters are a = 1.99 + 0.4*sin(t/f1) + 0.05*cos(t/f2) and c = 0.80 "
               "+ 0.15*cos(t/f1) + 0.05*sin(t/f2). Two coupled oscillations therefore move the map through a range "
               "of shapes. The f1 and f2 controls are divisors of the phase, so larger values slow their respective "
               "variation. Keep them positive: zero makes those phase expressions undefined. Parameters remain fixed "
               "during one batch of points and change for the next batch.\n"
               "\n"
               "The state is projected into the image using two angles, theta and phi. Their per-batch increments "
               "are the dtheta and dphi controls. These rotations reveal different views of the delayed-coordinate "
               "structure, although the final output is a two-dimensional projection. Setting both increments to "
               "zero holds the view still while the map continues evolving. The palette advances for each point, "
               "recording traversal order through the orbit.\n"
               "\n"
               "Iterations kcount produces kcount*1024 points per frame. More points fill the structure faster, at "
               "greater CPU cost. Cycles before reinit controls when the image is cleared and the orbit, phase "
               "counter, and view angles restart. The initial state is i=j=b=0.1.\n"
               "\n"
               "Further reading:\n"
               "https://en.wikipedia.org/wiki/Chaos_theory\n"
               "https://en.wikipedia.org/wiki/Attractor\n"
               "https://en.wikipedia.org/wiki/Lissajous_curve";
    }

    Thornbird()
        : Art("thornbird --- continuously varying Thornbird set") {
		}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;
    virtual void init_thornbird();

    void draw_thornbird_1();
    thornbirdstruct thornbird = {};

    int cycles = 1024*64;
    int count = 1024;
	int kcount = 32;
};
