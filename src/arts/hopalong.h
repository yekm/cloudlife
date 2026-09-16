#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"
#include <string>



typedef struct {
	int         centerx, centery;	/* center of the screen */
	double      a, b, c, d;
	double      i, j;	/* hopalong parameters */
	int         inc;
	//int         pix;
	int         op;
	int         count;
	int         scale;
	//int         bufsize;
} hopstruct;

class Hopalong : public Art {
public:
    std::string about() const override
    {
        return "Hopalong draws real-plane fractal orbits with a family of nonlinear recurrence rules. The code "
               "carries Patrick J. Naughton's 1991 copyright; its history records HOPALONG routines coded on 23 "
               "March 1988 from Scientific American, September 1986, page 14, where the construction was attributed "
               "to Barry Martin of Aston University. Later additions include Martin's sine variant in December 1994, "
               "Peter de Jong's map in July 1995 (Scientific American, July 1987, page 111), and Ed Kubaitis's EJK "
               "functions plus Renaldo Recuerdo's generalized exponent rule from xmartin2.2 in June 1997. The "
               "xscreensaver adaptation is archived in arχiv/xscreensaver/hacks/hopalong.c. Cloudlife's comments "
               "credit Pavel Vasilyev's Dear ImGui port in September 2023, and README.md documents these earlier "
               "credits.\n"
               "\n"
               "Each batch iterates a state (i,j) using randomized parameters chosen at initialization. In the "
               "original square-root option, with batch-dependent offset t, j' = a-i and i' = "
               "j-sign(i)*sqrt(abs(b*(i+t)-c)), using the nonnegative sign branch at zero. Display coordinates are "
               "proportional to (i'+j', i'-j'), giving a rotated view. The offset changes between batches, so the "
               "display explores a succession of related orbits rather than sampling one strictly fixed map forever.\n"
               "\n"
               "The op control maps numbers to these rules: 0 Martin square root; 1 EJK1 signed linear feedback; 2 "
               "EJK2 logarithmic feedback; 3 EJK4 sine/square-root branches; 4 EJK5 sine/linear branches; 5 Recuerdo "
               "absolute-power feedback; 6 de Jong; 7 popcorn; 8 Martin sine; 9 EJK3 signed sine; 10 EJK6 "
               "inverse-sine feedback of a fractional value. EJK variants generally keep j'=a-i while changing how "
               "the previous j and shifted i form i'. Recuerdo replaces the square root with abs(b*(i+t)-c)^d.\n"
               "\n"
               "The de Jong option combines sin(a*j)-cos(b*i) and sin(c*i)-cos(d*j), with a small batch offset in "
               "one argument. Popcorn instead applies i' = i-0.05*sin(j+tan(3*j)) and j' = j-0.05*sin(i+tan(3*i)), "
               "periodically advancing the initial point over a grid. These nonlinear updates can create detailed "
               "islands and filaments; some randomized parameter choices produce sparse or uninteresting pictures.\n"
               "\n"
               "Iterations kcount controls points per batch in units of 1024; iterations per frame controls the "
               "batch count. Cycles limit restarts with new random parameters. Changing op also restarts the orbit. "
               "More iterations accelerate filling at higher drawing cost, while palette position provides color "
               "across the generated points.\n"
               "\n"
               "Further reading:\n"
               "https://iacopoapps.appspot.com/hopalongwebgl/\n"
               "https://examples.holoviz.org/gallery/attractors/attractors.html\n"
               "https://en.wikipedia.org/wiki/Chaos_theory\n"
               "https://en.wikipedia.org/wiki/Attractor";
    }

    Hopalong()
    : Art("hopalong --- real plane fractals") {
        useVertex();
    }

private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    void init_hop();
    void draw_hop();

    hopstruct hps = {};

    int cycles = 1024*2;
    int count = 1024*16;
	int kcount = 16;
    int op = 0;
    int iterations = 10;
};
