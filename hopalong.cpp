/* -*- Mode: C; tab-width: 4 -*- */
/* hop --- real plane fractals */

#if 0
static const char sccsid[] = "@(#)hop.c	5.00 2000/11/01 xlockmore";
#endif

/*-
 * Copyright (c) 1991 by Patrick J. Naughton.
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
 * Revision History:
 * Changes in xlockmore distribution
 * 01-Nov-2000: Allocation checks
 * 24-Jun-1997: EJK and RR functions stolen from xmartin2.2
 *              Ed Kubaitis <ejk@ux2.cso.uiuc.edu> ejk functions and xmartin
 *              Renaldo Recuerdo rr function, generalized exponent version
 *              of the Barry Martin's square root function
 * 10-May-1997: Compatible with xscreensaver
 * 27-Jul-1995: added Peter de Jong's hop from Scientific American
 *              July 87 p. 111.  Sometimes they are amazing but there are a
 *              few duds (I did not see a pattern in the parameters).
 * 29-Mar-1995: changed name from hopalong to hop
 * 09-Dec-1994: added Barry Martin's sine hop
 * Changes in original xlock
 * 29-Oct-1990: fix bad (int) cast.
 * 29-Jul-1990: support for multiple screens.
 * 08-Jul-1990: new timing and colors and new algorithm for fractals.
 * 15-Dec-1989: Fix for proper skipping of {White,Black}Pixel() in colors.
 * 08-Oct-1989: Fixed long standing typo bug in RandomInitHop();
 *	            Fixed bug in memory allocation in init_hop();
 *	            Moved seconds() to an extern.
 *	            Got rid of the % mod since .mod is slow on a sparc.
 * 20-Sep-1989: Lint.
 * 31-Aug-1988: Forked from xlock.c for modularity.
 * 23-Mar-1988: Coded HOPALONG routines from Scientific American Sept. 86 p. 14.
 *              Hopalong was attributed to Barry Martin of Aston University
 *              (Birmingham, England)
 * 
 * 
 * Dear ImGui port by Pavel Vasilyev <yekm@299792458.ru>, Sep 2023
 */


#include "imgui_elements.h"
#include "hopalong.h"
#include "random.h"

#include <math.h>

#define w easel->w
#define h easel->h


#define MARTIN 0
#define POPCORN 7
#define SINE 8
#define EJK1 1
#define EJK2 2
#define EJK3 9
#define EJK4 3
#define EJK5 4
#define EJK6 10
#define RR 5
#define JONG 6
#ifdef OFFENDING
#define OPS 8			/* 8, 9, 10 might be too close to a swastika for some... */
#else
#define OPS 11
#endif


void Hopalong::init_hop()
{
	hopstruct *hp = &hps;
	double      range;

	hp->centerx = w / 2;
	hp->centery = h / 2;
	/* Make the other operations less common since they are less interesting */

	range = sqrt((double) hp->centerx * hp->centerx +
	     (double) hp->centery * hp->centery) / (1.0 + LRAND() / MAXRAND);
	hp->i = hp->j = 0.0;
	hp->inc = (int) ((LRAND() / MAXRAND) * 200) - 100;
#undef XMARTIN
	switch (hp->op) {
		case MARTIN:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 1500.0 + 40.0;
			hp->b = (LRAND() / MAXRAND) * 17.0 + 3.0;
			hp->c = (LRAND() / MAXRAND) * 3000.0 + 100.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 20.0;
			hp->b = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 20.0;
			if (LRAND() & 1)
				hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 20.0;
			else
				hp->c = 0.0;
#endif
			//(void) fprintf(stdout, "sqrt a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK1:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 500.0;
			hp->c = (LRAND() / MAXRAND) * 100.0 + 10.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 30.0;
			hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 40.0;
#endif
			hp->b = (LRAND() / MAXRAND) * 0.4;
			//(void) fprintf(stdout, "ejk1 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK2:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 500.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 30.0;
#endif
			hp->b = pow(10.0, 6.0 + (LRAND() / MAXRAND) * 24.0);
			if (LRAND() & 1)
				hp->b = -hp->b;
			hp->c = pow(10.0, (LRAND() / MAXRAND) * 9.0);
			if (LRAND() & 1)
				hp->c = -hp->c;
			//(void) fprintf(stdout, "ejk2 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK3:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 500.0;
			hp->c = (LRAND() / MAXRAND) * 80.0 + 30.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 30.0;
			hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 70.0;
#endif
			hp->b = (LRAND() / MAXRAND) * 0.35 + 0.5;
			//(void) fprintf(stdout, "ejk3 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK4:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 1000.0;
			hp->c = (LRAND() / MAXRAND) * 40.0 + 30.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 2.0;
			hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 200.0;
#endif
			hp->b = (LRAND() / MAXRAND) * 9.0 + 1.0;
			//(void) fprintf(stdout, "ejk4 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK5:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 600.0;
			hp->c = (LRAND() / MAXRAND) * 90.0 + 20.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 2.0;
			hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 200.0;
#endif
			hp->b = (LRAND() / MAXRAND) * 0.3 + 0.1;
			//(void) fprintf(stdout, "ejk5 a=%g, b=%g, c=%g\n", hp->a, hp->b, hp->c);
			break;
		case EJK6:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 100.0 + 550.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 30.0;
#endif
			hp->b = (LRAND() / MAXRAND) + 0.5;
			//(void) fprintf(stdout, "ejk6 a=%g, b=%g\n", hp->a, hp->b);
			break;
		case RR:
#ifdef XMARTIN
			hp->a = (LRAND() / MAXRAND) * 100.0;
			hp->b = (LRAND() / MAXRAND) * 20.0;
			hp->c = (LRAND() / MAXRAND) * 200.0;
#else
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 40.0;
			hp->b = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 200.0;
			hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * range / 20.0;
#endif
			hp->d = (LRAND() / MAXRAND) * 0.9;
			//(void) fprintf(stdout, "rr a=%g, b=%g, c=%g, d=%g\n", hp->a, hp->b, hp->c, hp->d);
			break;
		case POPCORN:
			hp->a = 0.0;
			hp->b = 0.0;
			hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.24 + 0.25;
			hp->inc = 100;
			//(void) fprintf(stdout, "popcorn a=%g, b=%g, c=%g, d=%g\n", hp->a, hp->b, hp->c, hp->d);
			break;
		case JONG:
			hp->a = ((LRAND() / MAXRAND) * 2.0 - 1.0) * M_PI;
			hp->b = ((LRAND() / MAXRAND) * 2.0 - 1.0) * M_PI;
			hp->c = ((LRAND() / MAXRAND) * 2.0 - 1.0) * M_PI;
			hp->d = ((LRAND() / MAXRAND) * 2.0 - 1.0) * M_PI;
			//(void) fprintf(stdout, "jong a=%g, b=%g, c=%g, d=%g\n", hp->a, hp->b, hp->c, hp->d);
			break;
		case SINE:	/* MARTIN2 */
#ifdef XMARTIN
			hp->a = M_PI + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.07;
#else
			hp->a = M_PI + ((LRAND() / MAXRAND) * 2.0 - 1.0) * 0.7;
#endif
			//(void) fprintf(stdout, "sine a=%g\n", hp->a);
			break;
	}

	hp->count = 0;
}


void Hopalong::draw_hop()
{
	double      oldj, oldi;
	int         k = count;
	hopstruct  *hp = &hps;

	typedef struct {
		int x, y;
	} point;
	point p;
	point *xp = &p;

	hp->inc++;
	while (k--) {
		oldj = hp->j;
		switch (hp->op) {
			case MARTIN:	/* SQRT, MARTIN1 */
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj + ((hp->i < 0)
					   ? sqrt(fabs(hp->b * oldi - hp->c))
					: -sqrt(fabs(hp->b * oldi - hp->c)));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK1:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? (hp->b * oldi - hp->c) :
						-(hp->b * oldi - hp->c));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK2:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i < 0) ? log(fabs(hp->b * oldi - hp->c)) :
					   -log(fabs(hp->b * oldi - hp->c)));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK3:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? sin(hp->b * oldi) - hp->c :
						-sin(hp->b * oldi) - hp->c);
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK4:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? sin(hp->b * oldi) - hp->c :
					  -sqrt(fabs(hp->b * oldi - hp->c)));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK5:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i > 0) ? sin(hp->b * oldi) - hp->c :
						-(hp->b * oldi - hp->c));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case EJK6:
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - asin((hp->b * oldi) - (long) (hp->b * oldi));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case RR:	/* RR1 */
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - ((hp->i < 0) ? -pow(fabs(hp->b * oldi - hp->c), hp->d) :
				     pow(fabs(hp->b * oldi - hp->c), hp->d));
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
			case POPCORN:
#define HVAL 0.05
#define INCVAL 50
				{
					double      tempi, tempj;

					if (hp->inc >= 100)
						hp->inc = 0;
					if (hp->inc == 0) {
						if (hp->a++ >= INCVAL) {
							hp->a = 0;
							if (hp->b++ >= INCVAL)
								hp->b = 0;
						}
						hp->i = (-hp->c * INCVAL / 2 + hp->c * hp->a) * M_PI / 180.0;
						hp->j = (-hp->c * INCVAL / 2 + hp->c * hp->b) * M_PI / 180.0;
					}
					tempi = hp->i - HVAL * sin(hp->j + tan(3.0 * hp->j));
					tempj = hp->j - HVAL * sin(hp->i + tan(3.0 * hp->i));
					xp->x = hp->centerx + (int) (w / 40 * tempi);
					xp->y = hp->centery + (int) (h / 40 * tempj);
					hp->i = tempi;
					hp->j = tempj;
				}
				break;
			case JONG:
				if (hp->centerx > 0)
					oldi = hp->i + 4 * hp->inc / hp->centerx;
				else
					oldi = hp->i;
				hp->j = sin(hp->c * hp->i) - cos(hp->d * hp->j);
				hp->i = sin(hp->a * oldj) - cos(hp->b * oldi);
				xp->x = hp->centerx + (int) (hp->centerx * (hp->i + hp->j) / 4.0);
				xp->y = hp->centery - (int) (hp->centery * (hp->i - hp->j) / 4.0);
				break;
			case SINE:	/* MARTIN2 */
				oldi = hp->i + hp->inc;
				hp->j = hp->a - hp->i;
				hp->i = oldj - sin(oldi);
				xp->x = hp->centerx + (int) (hp->i + hp->j);
				xp->y = hp->centery - (int) (hp->i - hp->j);
				break;
		}
        //xp->width = xp->height = hp->scale;
		//xp++;
		drawdot(xp->x, xp->y, easel->pal.get_color(count - k));
	}
}


bool Hopalong::render(uint32_t *p)
{
	hopstruct *hp = &hps;
	int i;

	for (i = 0; i < iterations; i++) {
		draw_hop();
		hp->count++;
	}

	if (hp->count > cycles) {
		resize(w, h);
	}

	return false;
}


bool Hopalong::render_gui ()
{
	bool up = false;
	hopstruct *hp = &hps;

	ScrollableSliderInt("cycles limit", &cycles, 0, 1024*10, "%d", 256);
	ScrollableSliderInt("iterations per frame", &iterations, 1, 256, "%d", 1);
	up |= ScrollableSliderInt("op", &hp->op, 0, 10, "%d", 1);
	if (ScrollableSliderInt("iterations kcount", &kcount, 0, 1024, "%d", 1)) {
		count = kcount * 1024;
		easel->pal.rescale(count);
	}

	ImGui::Text("hp->count %d, op %d", hp->count, hp->op);


	if (up) {
		resize(w, h);
	}

	return up;
}

void Hopalong::resize(int _w, int _h) {
	default_resize(_w, _h);

	easel->pal.rescale(count);

	init_hop();
}
