#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"
#include <string>



enum ftypes {
	SQRT, BIRDIE, STANDARD, TRIG, CUBIC, HENON, AILUJ, HSHOE, DELOG
};

typedef struct {
	int         maxx;
	int         maxy;	/* max of the screen */
	double      a;
	double      b;
	double      c;
	double      d;
	double      e;
	double      i;
	double      j;		/* discrete parameters */
	double      ic;
	double      jc;
	double      is;
	double      js;
	int         inc;
	enum ftypes op;
	int         count;

    int sqrt_sign, std_sign;

} discretestruct;


class Discrete : public Art {
public:
    std::string about() const override
    {
        return "Discrete explores the geometry of nine discrete dynamical systems: a point is repeatedly replaced by "
               "the result of a nonlinear map, and the visited positions build up a colored orbit. The source is Tim "
               "Auckland's 1996 discrete.c, adapted from Patrick J. Naughton's 1991 hop.c on 8 August 1996 and "
               "ported to xlockmore-4 on 31 July 1997. The xscreensaver version is preserved in "
               "arχiv/xscreensaver/hacks/discrete.c. Cloudlife's source credits the Dear ImGui port to Pavel "
               "Vasilyev, 27 May 2023; README.md also records the Auckland/Naughton lineage.\n"
               "\n"
               "Bias chooses the map in this order: 0 square-root Hopalong; 1 Bird in a Thornbush; 2 standard map; 3 "
               "trigonometric rotation; 4 cubic map; 5 Henon map; 6 inverse Julia iteration (AILUJ); 7 horseshoe; 8 "
               "delayed logistic map. These are distinct experiments, rather than different renderings of one "
               "equation. Most parameters are selected at initialization, so restarting can produce a new orbit.\n"
               "\n"
               "Writing the previous state as (x,y), the initialized Henon option uses "
               "x' = y + 1 - 1.4*x*x and y' = 0.3*x. The standard option applies "
               "y' = (1-a)*y + b*sin(x) + a*c, then x' = x + y', reducing both coordinates modulo 2*pi; "
               "a and c are initialized to zero, giving the usual undamped standard map. "
               "successive drawing batches also sample different starting momenta. The cubic option is x' = y, y' = "
               "2.77*y - y*y*y - b*x. The trigonometric option rotates (x,y) through angle x*x+y*y, contracts it by "
               "b, and adds 5 to x. Delayed logistic uses x' = 2.176399*x*(1-y), y' = x. These feedback rules can "
               "produce filaments, islands, repeating orbits, or escape.\n"
               "\n"
               "Hopalong uses a signed square root and samples new starting positions between batches. Bird in a "
               "Thornbush retains a third delayed coordinate and applies a cosine feedback rule. Inverse Julia "
               "iteration randomly chooses one of the two square roots of z-c, tracing a quadratic Julia set; its "
               "parameter receives a short Mandelbrot-style boundedness check. The horseshoe option starts from "
               "points around a square boundary and repeatedly stretches and folds them, with the displayed "
               "iteration depth cycling across batches.\n"
               "\n"
               "Count sets points per batch; iterations sets batches per frame; cycles limits the number of batches "
               "before clearing and reinitializing. Map-specific offsets and scales fit coordinates to the image, "
               "and color follows position within each batch. Increasing workload reveals finer orbit structure but "
               "increases drawing cost.\n"
               "\n"
               "Further reading:\n"
               "https://en.wikipedia.org/wiki/H%C3%A9non_map\n"
               "https://en.wikipedia.org/wiki/Standard_map\n"
               "https://en.wikipedia.org/wiki/Julia_set\n"
               "https://en.wikipedia.org/wiki/Chaos_theory";
    }

    Discrete()
        : Art("discrete --- chaotic mappings") {
			useVertex();
		}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;
    
	void init_discrete();
    void draw_discrete_1();
    discretestruct discrete = {};

    int cycles = 1024*2;
    int count = 1024*4;
    int bias = 0;
    int iterations = 10;
};
