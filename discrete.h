#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"



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
    Discrete()
        : Art("discrete --- chaotic mappings") {
			pixel_buffer_maximum = 1024*128;
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
    PaletteSetting pal;

};
