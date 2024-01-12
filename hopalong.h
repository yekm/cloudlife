#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"



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
