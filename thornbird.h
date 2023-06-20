#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"


typedef struct {
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
	  double  f1;
	  double  f2;
	}           liss;
    struct {
	  double  theta;
	  double  dtheta;
	  double  phi;
	  double  dphi;
	}           tumble;
    int         inc;
	int         count;
} thornbirdstruct;


class Thornbird : public Art {
public:
    Thornbird()
        : Art("thornbird --- continuously varying Thornbird set") {
			//pb = std::make_unique<PixelBuffer>();
			use_pixel_buffer = true;
			pixel_buffer_maximum = 1024*512;
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
    PaletteSetting pal;

};
