/* -*- Mode: C; tab-width: 4 -*- */
/* discrete --- chaotic mappings */

/*-
 * Copyright (c) 1996 by Tim Auckland <tda10.geo@yahoo.com>
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * "discrete" shows a number of fractals based on the "discrete map"
 * type of dynamical systems.  They include a different way of looking
 * at the HOPALONG system, an inverse julia-set iteration, the "Standard
 * Map" and the "Bird in a Thornbush" fractal.
 *
 * Revision History:
 * 27-May-2023: Dear ImGui port by Pavel Vasilyev <yekm@299792458.ru>
 * 01-Nov-2000: Allocation checks
 * 31-Jul-1997: Ported to xlockmore-4
 * 08-Aug-1996: Adapted from hop.c Copyright (c) 1991 by Patrick J. Naughton.
 */

#include "imgui_elements.h"
#include "discrete.h"
#include "random.h"

#include <math.h>

void Discrete::init_discrete()
{
	discrete = discretestruct{};
	double      range;
	discretestruct *hp = &discrete;

	hp->maxx = easel->w;
	hp->maxy = easel->h;
	hp->op = (ftypes)bias;
	switch (hp->op) {
		case HSHOE:
			hp->ic = 0;
			hp->jc = 0;
			hp->is = hp->maxx / (4);
			hp->js = hp->maxy / (4);
			hp->a = 0.5;
			hp->b = 0.5;
			hp->c = 0.2;
			hp->d = -1.25;
			hp->e = 1;
			hp->i = hp->j = 0.0;
			break;
		case DELOG:
			hp->ic = 0.5;
			hp->jc = 0.3;
			hp->is = hp->maxx / 1.5;
			hp->js = hp->maxy / 1.5;
			hp->a = 2.176399;
			hp->i = hp->j = 0.01;
			break;
		case HENON:
			hp->jc = ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.4;
			hp->ic = 1.3 * (1 - (hp->jc * hp->jc) / (0.4 * 0.4));
			hp->is = hp->maxx;
			hp->js = hp->maxy * 1.5;
			hp->a = 1;
			hp->b = 1.4;
			hp->c = 0.3;
			hp->i = hp->j = 0;
			break;
		case SQRT:
			hp->ic = 0;
			hp->jc = 0;
			hp->is = 1;
			hp->js = 1;
			range = sqrt((double) hp->maxx * 2 * hp->maxx * 2 +
				     (double) hp->maxy * 2 * hp->maxy * 2) /
				(10.0 + LRAND() % 10);

			hp->a = (LRAND() / MAXRAND) * range - range / 2.0;
			hp->b = (LRAND() / MAXRAND) * range - range / 2.0;
			hp->c = (LRAND() / MAXRAND) * range - range / 2.0;
			if (!(LRAND() % 2))
				hp->c = 0.0;
			hp->i = hp->j = 0.0;
			break;
		case STANDARD:
			hp->ic = M_PI;
			hp->jc = M_PI;
			hp->is = hp->maxx / (M_PI * 2);
			hp->js = hp->maxy / (M_PI * 2);
			hp->a = 0;	/* decay */
			hp->b = (LRAND() / MAXRAND) * 2.0;
			hp->c = 0;
			hp->i = M_PI;
			hp->j = M_PI;
			break;
		case BIRDIE:
			hp->ic = 0;
			hp->jc = 0;
			hp->is = hp->maxx / 2;
			hp->js = hp->maxy / 2;
			hp->a = 1.99 + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.2;
			hp->b = 0;
			hp->c = 0.8 + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.1;
			hp->i = hp->j = 0;
			break;
		case TRIG:
			hp->a = 5;
			hp->b = 0.5 + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.3;
			hp->ic = hp->a;
			hp->jc = 0;
			hp->is = hp->maxx / (hp->b * 20);
			hp->js = hp->maxy / (hp->b * 20);
			hp->i = hp->j = 0;
			break;
		case CUBIC:
			hp->a = 2.77;
			hp->b = 0.1 + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.1;
			hp->ic = 0;
			hp->jc = 0;
			hp->is = hp->maxx / 4;
			hp->js = hp->maxy / 4;
			hp->i = hp->j = 0.1;
			break;
		case AILUJ:
			{
				int         i;
				double      x, y, xn, yn;

				hp->ic = 0;
				hp->jc = 0;
				hp->is = hp->maxx / 4;
				hp->js = hp->maxx / 4;
				do {
					hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * 1.5 - 0.5;
					hp->b = ((LRAND() / MAXRAND) * 2.0 - 1.0) * 1.5;
					x = y = 0;
#define MAXITER 10
					for (i = 0; i < MAXITER && x * x + y * y < 13; i++) {	/* 'Brot calc */
						xn = x * x - y * y + hp->a;
						yn = 2 * x * y + hp->b;
						x = xn;
						y = yn;
					}
				} while (i < MAXITER);	/* wait for a connected set */
				hp->i = hp->j = 0.1;
				break;
			}
	}
	hp->inc = 0;

	easel->pal.rescale(count);

	hp->count = 0;
    hp->sqrt_sign = 1;
    hp->std_sign = 1;
}

void Discrete::draw_discrete_1()
{
	double      oldj, oldi;
	int         k = count;
	discretestruct *hp = &discrete;
	unsigned x, y;
	double      sint, cost, sinp, cosp;

	hp->inc++;

	while (k--) {
		oldj = hp->j;
		oldi = hp->i;
		switch (hp->op) {
			case HSHOE:
				{
					int         i;
#define HD
#ifdef HD
					if (k < count / 4) {
						hp->i = ((double) k / count) * 8 - 1;
						hp->j = 1;
					} else if (k < count / 2) {
						hp->i = 1;
						hp->j = 3 - ((double) k / count) * 8;
					} else if (k < 3 * count / 4) {
						hp->i = 5 - ((double) k / count) * 8;
						hp->j = -1;
					} else {
						hp->i = -1;
						hp->j = ((double) k / count) * 8 - 7;
					}
					for (i = 1; i < (hp->inc % 15); i++) {
						oldj = hp->j;
						oldi = hp->i;
#endif
						hp->i = (hp->a * oldi + hp->b) * oldj;
						hp->j = (hp->e - hp->d + hp->c * oldi) * oldj * oldj - hp->c * oldi + hp->d;
#ifdef HD
					}
#endif
					break;
				}
			case DELOG:
				hp->j = oldi;
				hp->i = hp->a * oldi * (1 - oldj);
				break;
			case HENON:
				hp->i = oldj + hp->a - hp->b * oldi * oldi;
				hp->j = hp->c * oldi;
				break;
			case SQRT:
				if (k) {
					hp->j = hp->a + hp->i;
					hp->i = -oldj + (hp->i < 0
					? sqrt(fabs(hp->b * (hp->i - hp->c)))
							 : -sqrt(fabs(hp->b * (hp->i - hp->c))));
				} else {
					hp->i = (hp->sqrt_sign ? 1 : -1) * hp->inc * hp->maxx / cycles / 2;
					hp->j = hp->a + hp->i;
					hp->sqrt_sign = !hp->sqrt_sign;
				}
				break;
			case STANDARD:
				if (k) {
					hp->j = (1 - hp->a) * oldj + hp->b * sin(oldi) + hp->a * hp->c;
					hp->j = fmod(hp->j + 2 * M_PI, 2 * M_PI);
					hp->i = oldi + hp->j;
					hp->i = fmod(hp->i + 2 * M_PI, 2 * M_PI);
				} else {
					hp->j = M_PI + fmod((hp->std_sign ? 1 : -1) * hp->inc * 2 * M_PI / (cycles - 0.5), M_PI);
					hp->i = M_PI;
					hp->std_sign = !hp->std_sign;
				}
				break;
			case BIRDIE:
				hp->j = oldi;
				hp->i = (1 - hp->c) * cos(M_PI * hp->a * oldj) + hp->c * hp->b;
				hp->b = oldj;
				break;
			case TRIG:
				{
					double      r2 = oldi * oldi + oldj * oldj;

					hp->i = hp->a + hp->b * (oldi * cos(r2) - oldj * sin(r2));
					hp->j = hp->b * (oldj * cos(r2) + oldi * sin(r2));
				}
				break;
			case CUBIC:
				hp->i = oldj;
				hp->j = hp->a * oldj - oldj * oldj * oldj - hp->b * oldi;
				break;
			case AILUJ:
				hp->i = ((LRAND() < MAXRAND / 2) ? -1 : 1) *
					sqrt(((oldi - hp->a) +
					      sqrt((oldi - hp->a) * (oldi - hp->a) + (oldj - hp->b) * (oldj - hp->b))) / 2);
				if (hp->i < 0.00000001 && hp->i > -0.00000001)
					hp->i = (hp->i > 0.0) ? 0.00000001 : -0.00000001;
				hp->j = (oldj - hp->b) / (2 * hp->i);
				break;
		}
		x = hp->maxx / 2 + (int) ((hp->i - hp->ic) * hp->is);
		y = hp->maxy / 2 - (int) ((hp->j - hp->jc) * hp->js);
		drawdot(x, y, easel->pal.get_color(count - k));
	}

}

bool Discrete::render(uint32_t *p)
{
	discretestruct *hp = &discrete;
	int i;

	for (i = 0; i < iterations; i++) {
		draw_discrete_1();
		hp->count++;
	}

	if (hp->count > cycles) {
		resize(easel->w, easel->h);
	}

	return false;
}


bool Discrete::render_gui ()
{
	bool up = false;
	discretestruct *hp = &discrete;

	ScrollableSliderInt("cycles", &cycles, 0, 1024*10, "%d", 256);
	up |= ScrollableSliderInt("count", &count, 0, 1024*10, "%d", 256);
	ScrollableSliderInt("iterations", &iterations, 1, 256, "%d", 1);
	up |= ScrollableSliderInt("bias", &bias, 0, 8, "%d", 1);

	ImGui::Text("hp->count %d, bias %d", hp->count, hp->op);


	if (up) {
		resize(easel->w, easel->h);
	}

	return up;
}

void Discrete::resize(int _w, int _h) {
	clear();

	init_discrete();
}
