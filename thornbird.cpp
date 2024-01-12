/* -*- Mode: C; tab-width: 4 -*- */
/* thornbird --- continuously varying Thornbird set */

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
 * "thornbird" shows a view of the "Bird in a Thornbush" fractal,
 * continuously varying the three free parameters.
 *
 * Revision History:
 * 11-Jul-2023: Dear ImGui port by Pavel Vasilyev <yekm@299792458.ru>
 * 01-Nov-2000: Allocation checks
 * 04-Jun-1999: 3D tumble added by Tim Auckland
 * 31-Jul-1997: Adapted from discrete.c Copyright (c) 1996 by Tim Auckland
 */


#include "imgui_elements.h"
#include "thornbird.h"
#include "random.h"
// from yarandom.h
#define LRAND()         ((long) (xoshiro256plus() & 0x7fffffff))
#define MAXRAND         (2147483648.0) /* unsigned 1<<31 as a float */
#define balance_rand(v)	((LRAND()/MAXRAND*(v))-((v)/2))	/* random around 0 */

#include <math.h>

void Thornbird::init_thornbird()
{
	//thornbird = thornbirdstruct{};
	//double      range;
	thornbirdstruct *hp = &thornbird;

	count = kcount * 1024;

	hp->maxx = easel->w;
	hp->maxy = easel->h;
	hp->b = 0.1;
	hp->i = hp->j = 0.1;

	hp->inc = 0;

	/* select frequencies for parameter variation */
	//hp->liss.f1 = LRAND() % 5000;
	//hp->liss.f2 = LRAND() % 2000;

	/* choose random 3D tumbling */
	hp->tumble.theta = 0;
	hp->tumble.phi = 0;
	//hp->tumble.dtheta = balance_rand(0.001);
	//hp->tumble.dphi = balance_rand(0.005);

	hp->count = 0;

	easel->pal.rescale(count);
}

void Thornbird::draw_thornbird_1()
{
	double      oldj, oldi;
	int         k = count;
	thornbirdstruct *hp = &thornbird;
	unsigned    x, y;
	double      sint, cost, sinp, cosp;

		/* vary papameters */
	hp->a = 1.99 + (0.4 * sin(hp->inc / hp->liss.f1) +
					0.05 * cos(hp->inc / hp->liss.f2));
	hp->c = 0.80 + (0.15 * cos(hp->inc / hp->liss.f1) +
					0.05 * sin(hp->inc / hp->liss.f2));

	/* vary view */
	hp->tumble.theta += hp->tumble.dtheta;
	hp->tumble.phi += hp->tumble.dphi;
	sint = sin(hp->tumble.theta);
	cost = cos(hp->tumble.theta);
	sinp = sin(hp->tumble.phi);
	cosp = cos(hp->tumble.phi);

	while (k--) {
		oldj = hp->j;
		oldi = hp->i;

		hp->j = oldi;
		hp->i = (1 - hp->c) * cos(M_PI * hp->a * oldj) + hp->c * hp->b;
		hp->b = oldj;

		x = (short) (hp->maxx / 2 * (1
						+ sint*hp->j + cost*cosp*hp->i - cost*sinp*hp->b));
		y = (short) (hp->maxy / 2 * (1
						- cost*hp->j + sint*cosp*hp->i - sint*sinp*hp->b));

		drawdot(x, y, easel->pal.get_color(count - k));
	}

	hp->inc++;
}

bool Thornbird::render(uint32_t *p)
{
	thornbirdstruct *hp = &thornbird;

	draw_thornbird_1();
	hp->count++;

	if (hp->count > cycles) {
		resize(easel->w, easel->h);
	}

	return false;
}


bool Thornbird::render_gui ()
{
	bool up = false;
	thornbirdstruct *hp = &thornbird;

	ScrollableSliderInt("cycles before reinit", &cycles, 0, 1024*10, "%d", 256);
	ScrollableSliderDouble("hp->liss.f1",
		&hp->liss.f1, 0, 5000, "%.1f", 5);
	ScrollableSliderDouble("hp->liss.f2",
		&hp->liss.f2, 0, 2000, "%.1f", 2);
	ScrollableSliderDouble("hp->tumble.dtheta",
		&hp->tumble.dtheta, -0.002, 0.002, "%.4f", 0.0001);
	ScrollableSliderDouble("hp->tumble.dphi",
		&hp->tumble.dphi, -0.006, 0.006, "%.4f", 0.0001);

	if (ScrollableSliderInt("iterations kcount", &kcount, 0, 1024, "%d", 1)) {
		count = kcount * 1024;
		easel->pal.rescale(count);
	}

	ImGui::Text("hp->count %d", hp->count);


	if (up) {
		resize(easel->w, easel->h);
	}

	return false;
}

void Thornbird::resize(int _w, int _h) {
	clear();

	init_thornbird();
}
