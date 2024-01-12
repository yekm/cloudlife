/* xscreensaver, Copyright (c) 1992-2014 Jamie Zawinski <jwz@jwz.org>
 *
 *  reaction/diffusion textures
 *  Copyright (c) 1997 Scott Draves spot@transmeta.com
 *  this code is derived from Bomb
 *  see http://www.cs.cmu.edu/~spot/bomb.html
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * And remember: X Windows is to graphics hacking as roman numerals are to
 * the square root of pi.
 *
 * Dear ImGui port by Pavel Vasilyev <yekm@299792458.ru>, Jun 2023
 */

#include <math.h>

#include "random.h"
#include "rdbomb.h"

#include "imgui_elements.h"

// random from yarandom.h returns unsigend int
#define random xoshiro256plus

#define bps 16
#define mx ((1 << 16) - 1)

/* you can replace integer mults wish shift/adds with these,
   but it doesn't help on my 586 */
#define x5(n) ((n << 2) + n)
#define x7(n) ((n << 3) - n)

/* why strip bit? */
#define R (random() & ((1 << 30) - 1))
#define BELLRAND(x)                                                            \
  (((random() % (x)) + (random() % (x)) + (random() % (x))) / 3)

/* returns number of pixels that the pixack produces.  called once. */
void RDbomb::pixack_init() {
  easel->set_texture_size(width, height);
  npix = (width + 2) * (height + 2);
  r1.resize(npix);
  r2.resize(npix);
  r1b.resize(npix);
  r2b.resize(npix);
  fill0(r1);
  fill0(r2);
  fill0(r1b);
  fill0(r2b);

  RDbomb *st = this;
  int w2 = st->width + 2;
  int s;

  for (int i = 0; i < st->npix; i++) {
    /* equilibrium */
    st->r1[i] = 65500;
    st->r2[i] = 11;
  }

  s = w2 * (st->height / 2) + st->width / 2;
  switch(init_type) {
  case 0:
    for (int i = -st->radius; i < (st->radius + 1); i++)
      for (int j = -st->radius; j < (st->radius + 1); j++)
        st->r2[s + i + j * w2] = mx - (R & 63);
    break;
  case 1:
    st->r2[s + -st->radius + -st->radius * w2] = mx - (R & 63);
    st->r2[s + -st->radius +  st->radius * w2] = mx - (R & 63);
    st->r2[s +  st->radius + -st->radius * w2] = mx - (R & 63);
    st->r2[s +  st->radius +  st->radius * w2] = mx - (R & 63);
    break;
  case 2:
    for (int i = -st->radius; i < (st->radius + 1); i++)
      for (int j = -st->radius; j < (st->radius + 1); j++)
        if (R%1024 == 0)
          st->r2[s + i + j * w2] = mx - (R & 63);
    break;
  }
}


#define test_pattern_hyper 0

/* returns the pixels.  called many times. */
void RDbomb::pixack_frame() {
  RDbomb *st = this;
  int i, j;
  int w2 = st->width + 2;
  unsigned short *t;
#if test_pattern_hyper
  if (st->frame_number & 0x100)
    sleep(1);
#endif
#if 0
  if (!(st->frame_number % st->epoch_time)) {
    int s;

    for (i = 0; i < st->npix; i++) {
      /* equilibrium */
      st->r1[i] = 65500;
      st->r2[i] = 11;
    }

    s = w2 * (st->height / 2) + st->width / 2;
    {
      int maxr = st->width / 2 - 2;
      int maxr2 = st->height / 2 - 2;
      if (maxr2 < maxr)
        maxr = maxr2;

      if (st->radius < 0)
        st->radius = 1 + ((R % 10) ? (R % 5) : (R % maxr));
      if (st->radius > maxr)
        st->radius = maxr;
    }
    for (i = -st->radius; i < (st->radius + 1); i++)
      for (j = -st->radius; j < (st->radius + 1); j++)
        st->r2[s + i + j * w2] = mx - (R & 63);
    /*
    if (st->reaction < 0 || st->reaction > 2)
      st->reaction = R & 1;
    if (st->diffusion < 0 || st->diffusion > 2)
      st->diffusion = (R % 5) ? ((R % 3) ? 0 : 1) : 2;
    if (2 == st->reaction && 2 == st->diffusion)
      st->reaction = st->diffusion = 0;
    */
  }
#endif
  for (i = 0; i <= st->width + 1; i++) {
    st->r1[i] = st->r1[i + w2 * st->height];
    st->r2[i] = st->r2[i + w2 * st->height];
    st->r1[i + w2 * (st->height + 1)] = st->r1[i + w2];
    st->r2[i + w2 * (st->height + 1)] = st->r2[i + w2];
  }
  for (i = 0; i <= st->height + 1; i++) {
    st->r1[w2 * i] = st->r1[st->width + w2 * i];
    st->r2[w2 * i] = st->r2[st->width + w2 * i];
    st->r1[w2 * i + st->width + 1] = st->r1[w2 * i + 1];
    st->r2[w2 * i + st->width + 1] = st->r2[w2 * i + 1];
  }
  for (i = 0; i < st->height; i++) {
    int ii = i + 1;
    unsigned short *i1 = st->r1.data() + 1 + w2 * ii;
    unsigned short *i2 = st->r2.data() + 1 + w2 * ii;
    unsigned short *o1 = st->r1b.data() + 1 + w2 * ii;
    unsigned short *o2 = st->r2b.data() + 1 + w2 * ii;
    for (j = 0; j < st->width; j++) {
#if test_pattern_hyper
      int r1 = (i * j + (st->frame_number & 127) * frame) & 65535;
#else
      int uvv, r1 = 0, r2 = 0;
      switch (st->diffusion) {
      case 0:
        r1 = i1[j] + i1[j+1] + i1[j-1] + i1[j+w2] + i1[j-w2];
        r1 = r1 / 5;
        r2 = (i2[j]<<3) + i2[j+1] + i2[j-1] + i2[j+w2] + i2[j-w2];
        r2 = r2 / 12;
        break;
      case 1:
        r1 = i1[j+1] + i1[j-1] + i1[j+w2] + i1[j-w2];
        r1 = r1 >> 2;
        r2 = (i2[j]<<2) + i2[j+1] + i2[j-1] + i2[j+w2] + i2[j-w2];
        r2 = r2 >> 3;
        break;
      case 2:
        r1 = (i1[j]<<1) + (i1[j+1]<<1) + (i1[j-1]<<1) + i1[j+w2] + i1[j-w2];
        r1 = r1 >> 3;
        r2 = (i2[j]<<2) + i2[j+1] + i2[j-1] + i2[j+w2] + i2[j-w2];
        r2 = r2 >> 3;
        break;
      }

      /* John E. Pearson "Complex Patterns in a Simple System"
         Science, July 1993 */

      /* uvv = (((r1 * r2) >> bps) * r2) >> bps; */
      /* avoid signed integer overflow */
      uvv = ((((r1 >> 1) * r2) >> bps) * r2) >> (bps - 1);
      switch (st->reaction) { /* costs 4% */
      case 0:
        r1 += 4 * (((28 * (mx-r1)) >> 10) - uvv);
        r2 += 4 * (uvv - ((80 * r2) >> 10));
        break;
      case 1:
        r1 += 3 * (((27 * (mx-r1)) >> 10) - uvv);
        r2 += 3 * (uvv - ((80 * r2) >> 10));
        break;
      case 2:
        r1 += 2 * (((28 * (mx-r1)) >> 10) - uvv);
        r2 += 3 * (uvv - ((80 * r2) >> 10));
        break;
      }
      if (r1 > mx) r1 = mx;
      if (r2 > mx) r2 = mx;
      if (r1 < 0) r1 = 0;
      if (r2 < 0) r2 = 0;
      o1[j] = r1;
      o2[j] = r2;
#endif

#if dither_when_mapped
      //qqq[j] = st->colors[st->mc[r1] % st->ncolors].pixel;
#else
      qqq[j] = st->colors[(r1 >> 8) % st->ncolors].pixel;
#endif
      drawdot(j, i, easel->pal.get_color(r1 % st->ncolors));
    }
  }

  std::swap(st->r1, st->r1b);
  std::swap(st->r2, st->r2b);
}

/* should factor into RD-specfic and compute-every-pixel general */
void RDbomb::rd_init() {
  pixack_init();

#if 0
  st->colors = (XColor *) malloc(sizeof(*st->colors) * (st->ncolors+1));

  st->mapped = (vdepth <= 8 &&
                has_writable_cells(st->xgwa.screen, st->xgwa.visual));

  {
    int i, di;
    st->mc = (unsigned char *) malloc(1<<16);
    for (i = 0; i < (1<<16); i++) {
      di = (i + (random()&255))>>8;
      if (di > 255) di = 255;
      st->mc[i] = di;
    }
  }
#endif
}

bool RDbomb::render(uint32_t *p) {
  /* Let's compute N frames at once. This speeds up the progress of
     the animation and the seething, but doesn't appreciably affect the
     frame rate or CPU utilization. */
  for (int ii = 0; ii < iterations; ii++) {
    pixack_frame();
  }

  return false;
}

bool RDbomb::render_gui() {
  bool up = false;

  ScrollableSliderInt("ncolors", &ncolors, 1, 1024 * 4, "%d", 256);
  ScrollableSliderInt("iterations", &iterations, 0, 16, "%d", 1);
  up |= ScrollableSliderInt("radius", &radius, 0, std::min(width, height)/2-2, "%d", 1);
  up |= ScrollableSliderInt("init type", &init_type, 0, 2, "%d", 1);
  ScrollableSliderInt("reaction", &reaction, 0, 2, "%d", 1);
  ScrollableSliderInt("diffusion", &diffusion, 0, 2, "%d", 1);

  up |= ScrollableSliderInt("width", &width, 10, 1024, "%d", 1);
  up |= ScrollableSliderInt("height", &height, 10, 1024, "%d", 1);

  return up;
}

void RDbomb::resize(int _w, int _h) {
  clear();
  easel->pal.rescale(ncolors);
  rd_init();
}
