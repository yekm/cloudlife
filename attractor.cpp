
#include "imgui_elements.h"
#include "attractor.h"
#include "random.h"
// from yarandom.h
#define LRAND()         ((long) (xoshiro256plus() & 0x7fffffff))
#define MAXRAND         (2147483648.0) /* unsigned 1<<31 as a float */
#define balance_rand(v)	((LRAND()/MAXRAND*(v))-((v)/2))	/* random around 0 */

#include <math.h>

// https://examples.holoviz.org/attractors/attractors.html

void Attractor::init()
{
	int centerx, centery;
	centerx = w / 2;
	centery = h / 2;
	
	double range = sqrt((double) centerx * centerx +
	     (double) centery * centery) / (1.0 + LRAND() / MAXRAND);
	ai = aj = 0.0;
	/*
	inc = (int) ((LRAND() / MAXRAND) * 200) - 100;
	a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 20.0;
	b = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 20.0;
	if (LRAND() & 1)
		c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 20.0;
	else
		c = 0.0;
	*/
}


void Attractor::draw()
{

}


bool Attractor::render(uint32_t *p)
{
	int points_per_frame = 1024*128;
	int x, y;
	double oldi, oldj;

	for (int i=0; i < points_per_frame && count < pixel_buffer_maximum; ++i, ++count) {
		oldj = aj;
		oldi = ai + inc;
		aj = a - ai;
		ai = oldj + (ai < 0
					    ?  sqrt(fabs(b * oldi - c))
					    : -sqrt(fabs(b * oldi - c))
					);
		x = w/2 + (int) (ai + aj);
		y = h/2 - (int) (ai - aj);

		drawdot(x, y, pal.get_color((pixel_buffer_maximum - count)>>2));
	}

	return false;
}


bool Attractor::render_gui ()
{
	bool up = false;

	//up |= ScrollableSliderInt("op", &op, 0, 10, "%d", 1);
	/*
	if (ScrollableSliderInt("iterations kcount", &kcount, 0, 1024, "%d", 1)) {
		count = kcount * 1024;
		pal.rescale(count >> 1);
	}
	*/

	up |= ScrollableSliderDouble("inc", &inc, -200, 200, "%.4f", 0.0001);
	up |= ScrollableSliderDouble("a", &a, -20, 20, "%.4f", 0.000001);
	up |= ScrollableSliderDouble("b", &b, -20, 20, "%.4f", 0.000001);
	up |= ScrollableSliderDouble("c", &c, -20, 20, "%.4f", 0.000001);
	up |= ScrollableSliderDouble("d", &d, -20, 20, "%.4f", 0.000001);


	up |= pal.RenderGui();

	//ImGui::Text("count %d, op %d", count, op);


	if (up) {
		resize(w, h);
	}

	return false;
}

void Attractor::resize(int _w, int _h) {
	default_resize(_w, _h);

	pal.rescale(pixel_buffer_maximum >> 2);
	count = 0;

	//clear();
	init();
}
