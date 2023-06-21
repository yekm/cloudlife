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
 */

#include <math.h>

#include "rdbomb.h"
#include "random.h"

// random from yarandom.h returns unsigend int
#define random xoshiro256plus
//static inline unsigned int random() {
//  return xoshiro256plus();
//}

/* -----------------------------------------------------------
   pixel hack, 8-bit pixel grid, first/next frame interface

   pixack_init(int *size_h, int *size_v)
   pixack_frame(char *pix_buf)
   */


#define bps 16
#define mx ((1<<16)-1)

/* you can replace integer mults wish shift/adds with these,
   but it doesn't help on my 586 */
#define x5(n) ((n<<2)+n)
#define x7(n) ((n<<3)-n)

/* why strip bit? */
#define R (random()&((1<<30)-1))
#define BELLRAND(x) (((random()%(x)) + (random()%(x)) + (random()%(x)))/3)

/* returns number of pixels that the pixack produces.  called once. */
void RDbomb::pixack_init()
{
  RDbomb * st = this;
  //st->width  = get_integer_resource (st->dpy, "width",  "Integer");
  //st->height = get_integer_resource (st->dpy, "height", "Integer");

  if (st->width <= 0 && st->height <= 0 && (R & 1))
    st->width = st->height = 64 + BELLRAND(512);

  if (st->width  <= 0) st->width  = 64 + BELLRAND(512);
  if (st->height <= 0) st->height = 64 + BELLRAND(512);

  //if (st->width  > w)  st->width  = w;
  //if (st->height > h) st->height = h;

  /* jwz: when (and only when) XSHM is in use on an SGI 8-bit visual,
     we get shear unless st->width is a multiple of 4.  I don't understand
     why.  This is undoubtedly the wrong fix... */
  //if (visual_depth (st->xgwa.screen, st->xgwa.visual) == 8)
  //  st->width &= ~0x7;

  /* don't go there */
  if (st->width < 10) st->width = 10;
  if (st->height < 10) st->height = 10;
  //st->epoch_time = get_integer_resource (st->dpy, "epoch", "Integer");
  st->npix = (st->width + 2) * (st->height + 2);
  st->r1.resize(st->npix);
  st->r2.resize(st->npix);
  st->r1b.resize(st->npix);
  st->r2b.resize(st->npix);
}

#define test_pattern_hyper 0


/* returns the pixels.  called many times. */
void RDbomb::pixack_frame()
{
  RDbomb * st = this;
  int i, j;
  int w2 = st->width + 2;
  unsigned short *t;
#if test_pattern_hyper
  if (st->frame&0x100)
    sleep(1);
#endif

  if (!(st->frame_number%st->epoch_time)) {
    int s;

    for (i = 0; i < st->npix; i++) {
      /* equilibrium */
      st->r1[i] = 65500;
      st->r2[i] = 11;
    }

    //random_colors(st);

    //XSetWindowBackground(st->dpy, st->window, st->colors[255 % st->ncolors].pixel);
    //XClearWindow(st->dpy, st->window);

    s = w2 * (st->height/2) + st->width/2;
    {
      int maxr = st->width/2-2;
      int maxr2 = st->height/2-2;
      if (maxr2 < maxr) maxr = maxr2;

      if (st->radius < 0)
	st->radius = 1 + ((R%10) ? (R%5) : (R % maxr));
      if (st->radius > maxr) st->radius = maxr;
    }
    for (i = -st->radius; i < (st->radius+1); i++)
      for (j = -st->radius; j < (st->radius+1); j++)
	st->r2[s + i + j*w2] = mx - (R&63);
    if (st->reaction < 0 || st->reaction > 2) st->reaction = R&1;
    if (st->diffusion < 0 || st->diffusion > 2)
      st->diffusion = (R%5) ? ((R%3)?0:1) : 2;
    if (2 == st->reaction && 2 == st->diffusion)
      st->reaction = st->diffusion = 0;
  }
  for (i = 0; i <= st->width+1; i++) {
    st->r1[i] = st->r1[i + w2 * st->height];
    st->r2[i] = st->r2[i + w2 * st->height];
    st->r1[i + w2 * (st->height + 1)] = st->r1[i + w2];
    st->r2[i + w2 * (st->height + 1)] = st->r2[i + w2];
  }
  for (i = 0; i <= st->height+1; i++) {
    st->r1[w2 * i] = st->r1[st->width + w2 * i];
    st->r2[w2 * i] = st->r2[st->width + w2 * i];
    st->r1[w2 * i + st->width + 1] = st->r1[w2 * i + 1];
    st->r2[w2 * i + st->width + 1] = st->r2[w2 * i + 1];
  }
  for (i = 0; i < st->height; i++) {
    int ii = i + 1;
    //char *q = pix_buf + st->width * i;
    //short *qq = ((short *) pix_buf) + st->width * i;
/*  long  *qqq = ((long *) pix_buf) + st->width * i;  -- crashes on Alpha */
    //int   *qqq = ((int  *) pix_buf) + st->width * i;
    unsigned short *i1 = st->r1 .data() + 1 + w2 * ii;
    unsigned short *i2 = st->r2 .data() + 1 + w2 * ii;
    unsigned short *o1 = st->r1b.data() + 1 + w2 * ii;
    unsigned short *o2 = st->r2b.data() + 1 + w2 * ii;
    for (j = 0; j < st->width; j++) {
#if test_pattern_hyper
      int r1 = (i * j + (st->frame&127)*frame)&65535;
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
      uvv = ((((r1 >> 1)* r2) >> bps) * r2) >> (bps - 1);
      switch (st->reaction) {  /* costs 4% */
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

      /* this is terrible.  here i want to assume ncolors = 256.
	 should lose double indirection */
#if 0
      if (st->mapped)
#if dither_when_mapped
	q[j] = st->colors[st->mc[r1]  % st->ncolors].pixel;
#else
      q[j] = st->colors[(r1>>8) % st->ncolors].pixel;
#endif
      else if (st->pdepth == 8)
	q[j] = st->colors[(r1>>8) % st->ncolors].pixel;
      else if (st->pdepth == 16)
#if dither_when_mapped
	qq[j] = st->colors[st->mc[r1] % st->ncolors].pixel;
#else
	qq[j] = st->colors[(r1>>8) % st->ncolors].pixel;
#endif
      else if (st->pdepth == 32)
#if dither_when_mapped
	qqq[j] = st->colors[st->mc[r1] % st->ncolors].pixel;
#else
	qqq[j] = st->colors[(r1>>8) % st->ncolors].pixel;
#endif
      else
	abort();
#endif

#if dither_when_mapped
  drawdot(i, j, pal.get_color(r1));
	//qqq[j] = st->colors[st->mc[r1] % st->ncolors].pixel;
#else
	qqq[j] = st->colors[(r1>>8) % st->ncolors].pixel;
#endif
    }

  }

  std::swap(st->r1, st->r1b);
  std::swap(st->r2, st->r2b);
  //t = st->r1; st->r1 = st->r1b; st->r1b = t;
  //t = st->r2; st->r2 = st->r2b; st->r2b = t;
}

/* should factor into RD-specfic and compute-every-pixel general */
void RDbomb::rd_init ()
{
  RDbomb * st = this;
  //int vdepth;

  pixack_init();
  {
    double s = size;
    double p = speed;
    if (s < 0.0 || s > 1.0)
      s = 1.0;
    s = sqrt(s);
    st->array_width = w * s;
    st->array_height = h * s;
    if (s < 0.99) {
      st->array_width = (st->array_width / st->width) * st->width;
      st->array_height = (st->array_height / st->height) * st->height;
    }
    if (st->array_width < st->width) st->array_width = st->width;
    if (st->array_height < st->height) st->array_height = st->height;
    st->array_x = (w - st->array_width)/2;
    st->array_y = (h - st->array_height)/2;
    st->array_dx = p;
    st->array_dy = .31415926 * p;

    /* start in a random direction */
    if (random() & 1) st->array_dx = -st->array_dx;
    if (random() & 1) st->array_dy = -st->array_dy;

  }
  st->npix = (st->width + 2) * (st->height + 2);

  st->pdepth = 32;
#if 0
/*  gcv.function = GXcopy;*/
  //st->gc = XCreateGC(st->dpy, win, 0 /*GCFunction*/, &gcv);
  //vdepth = visual_depth(DefaultScreenOfDisplay(st->dpy), st->xgwa.visual);
  vdepth = 32;

  /* This code only deals with pixmap depths of 1, 8, 16, and 32.
     Therefore, we assume that those depths will be supported by the
     coresponding visual depths (that depth-24 dpys accept depth-32
     pixmaps, and that depth-12 dpys accept depth-16 pixmaps.) */
  st->pdepth = (vdepth == 1 ? 1 :
	    vdepth <= 8 ? 8 :
	    vdepth <= 16 ? 16 :
	    32);

  /* Ok, this like, sucks and stuff.  There are some XFree86 systems
     that have depth-24 visuals, that do not accept depth-32 XImages!
     Which if you ask me is just absurd, since all it would take is
     for the server to truncate the bits in that case.  So, this crap
     here detects the specific case of: we have chosen depth 32;
     and the server does not support depth 32.  In that case, we
     try and use depth 16 instead.

     The real fix would be to rewrite this program to deal with
     depth 24 directly (or even better, arbitrary depths, but that
     would mean going through the XImage routines instead of messing
     with the XImage->data directly.)

     jwz, 18-Mar-99: well, the X servers I have access to these days do
     support 32-deep images on deep visuals, so I no longer have the
     ability to test this code -- but it was causing problems on the
     visuals that I do have, and I think that's because I mistakenly
     wrote `pfv[i].depth' when I meant to write `pfv[i].bits_per_pixel'.
     The symptom I was seeing was that the grid was 64x64, but the
     images were being drawn 32x32 -- so there was a black stripe on
     every other row.  Wow, this code sucks so much.
  */
  if (st->pdepth == 32)
    {
      int i, pfvc = 0;
      Bool ok = False;
      XPixmapFormatValues *pfv = XListPixmapFormats (st->dpy, &pfvc);
      for (i = 0; i < pfvc; i++)
        if (pfv[i].bits_per_pixel == st->pdepth)
          ok = True;
      if (!ok)
        st->pdepth = 16;
      free (pfv);
    }
  st->cmap = st->xgwa.colormap;
  st->ncolors = get_integer_resource (st->dpy, "colors", "Integer");

  if (st->ncolors <= 0 || st->ncolors >= 255) {
    if (vdepth > 8)
      st->ncolors = 2047;
    else
      st->ncolors = 255;
  }

  if (mono_p || st->ncolors < 2) st->ncolors = 2;
  if (st->ncolors <= 2) mono_p = True;
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

  st->image = create_xshm_image(st->dpy, st->xgwa.visual, vdepth,
                                ZPixmap, &st->shm_info, st->width, st->height);
  st->pd = st->image->data;

  return st;
#endif
}

bool RDbomb::render(uint32_t *p)
{
  RDbomb *st = this;
  bool bump = false;

  /* Let's compute N frames at once. This speeds up the progress of
     the animation and the seething, but doesn't appreciably affect the
     frame rate or CPU utilization. */
  int ii;
  int chunk = 3;
  for (ii = 0; ii < chunk; ii++) {

  int i, j;
  pixack_frame();
#if 0
  if (ii == chunk-1) {  /* Only need to putimage on the final frame */
  for (i = 0; i < st->array_width; i += st->width)
    for (j = 0; j < st->array_height; j += st->height)
      put_xshm_image(st->dpy, st->window, st->gc, st->image, 0, 0, i+st->array_x, j+st->array_y,
                     st->width, st->height, &st->shm_info);
  }
#endif

  st->array_x += st->array_dx;
  st->array_y += st->array_dy;
  if (st->array_x < 0) {
    st->array_x = 0;
    st->array_dx = -st->array_dx;
    bump = true;
  } else if (st->array_x > (w - st->array_width)) {
    st->array_x = (w - st->array_width);
    st->array_dx = -st->array_dx;
    bump = true;
  }
  if (st->array_y < 0) {
    st->array_y = 0;
    st->array_dy = -st->array_dy;
    bump = true;
  } else if (st->array_y > (h - st->array_height)) {
    st->array_y = (h - st->array_height);
    st->array_dy = -st->array_dy;
    bump = true;
  }

  if (bump) {
    if (random() & 1) {
      double swap = st->array_dx;
      st->array_dx = st->array_dy;
      st->array_dy = swap;
    }
  }

  //st->frame++;
  }

  return false;
}

bool RDbomb::render_gui()
{
	bool up = false;

	ScrollableSliderInt("ncolors", &ncolors, 1, 1024*4, "%d", 256);
  ScrollableSliderDouble("size", &size, 0, 10, "%.2f", 0.1);
  ScrollableSliderDouble("speed", &speed, 0, 10, "%.2f", 0.1);
  ScrollableSliderInt("epoch_time", &epoch_time, 128, 1024*100, "%d", 1024);
  ScrollableSliderInt("radius", &radius, -10, 10, "%d", 1);
  ScrollableSliderInt("reaction", &reaction, -10, 10, "%d", 1);
  ScrollableSliderInt("diffusion", &diffusion, -10, 10, "%d", 1);


	pal.RenderGui();

	if (up) {
		resize(w, h);
	}

	return false;
}

void RDbomb::resize(int _w, int _h) {
  default_resize(_w, _h);
  pal.rescale(ncolors);
  rd_init();
}
