/* Copyright © Chris Le Sueur and Robby Griffin, 2005-2006

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Ultimate thanks go to Massimino Pascal, who created the original
xscreensaver hack, and inspired me with it's swirly goodness. This
version adds things like variable quality, number of functions and also
a groovier colouring mode.

This version by Chris Le Sueur <thefishface@gmail.com>, Feb 2005
Many improvements by Robby Griffin <rmg@terc.edu>, Mar 2006
Multi-coloured mode added by Jack Grahl <j.grahl@ucl.ac.uk>, Jan 2007
Dear ImGui port by Pavel Vasilyev <yekm@299792458.ru>, May 2023
*/

#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <limits>

#include <cstdint>
#include <algorithm>

#include "imgui_elements.h"
#include "ifs.h"
#include "random.h"



#define getdot(x,y) (board[((y)*widthb)+((x)>>5)] &  (1<<((x) & 31)))
#define setdot(x,y) (board[((y)*widthb)+((x)>>5)] |= (1<<((x) & 31)))

static float
myrandom(float up)
{
  return (((float)xoshiro256plus() / std::numeric_limits<uint64_t>::max()) * up);
}

/* Set a point to be drawn, if it hasn't been already.
 * Expects coordinates in 256ths of a pixel. */
void
IFS::sp(int x, int y)
{
  if (x < 0 || x >= width8 || y < 0 || y >= height8)
    return;

  x >>= 8;
  y >>= 8;

  //if (getdot(x, y)) return;
  //setdot(x, y);

  drawdot(x, y, current_color);
  //drawbox(x, y, pscale, current_color);
}


/* Precompute integer values for matrix multiplication and vector
 * addition. The matrix multiplication will go like this (see iterate()):
 *   |x2|     |ua ub|   |x|     |utx|
 *   |  |  =  |     | * | |  +  |   |
 *   |y2|     |uc ud|   |y|     |uty|
 *
 * There is an extra factor of 2^10 in these values, and an extra factor of
 * 2^8 in the coordinates, in order to implement fixed-point arithmetic.
 */
void
IFS::lensmatrix(Lens *l)
{
  l->ua = 1024.0 * l->s * cos(l->r);
  l->ub = -1024.0 * l->s * sin(l->r);
  l->uc = -l->ub;
  l->ud = l->ua;
  l->utx = 131072.0 * easel->w * (l->s * (sin(l->r) - cos(l->r))
				   + l->tx / 16 + 1);
  l->uty = -131072.0 * easel->h * (l->s * (sin(l->r) + cos(l->r))
				     + l->ty / 16 - 1);
}

void
IFS::CreateLens(
           float nr,
           float ns,
           float nx,
           float ny,
           Lens *newlens)
{
  newlens->sa = newlens->txa = newlens->tya = 0;
  if (rotate) {
    newlens->r = newlens->ro = newlens->rt = nr;
    newlens->rc = 1;
  }
  else newlens->r = 0;

  if (scale) {
    newlens->s = newlens->so = newlens->st = ns;
    newlens->sc = 1;
  }
  else newlens->s = 0.5;

  newlens->tx = nx;
  newlens->ty = ny;

  lensmatrix(newlens);
}

void
IFS::mutate(Lens *l)
{
  if (rotate) {
    float factor;
    if(l->rc >= 1) {
      l->rc = 0;
      l->ro = l->rt;
      l->rt = myrandom(4) - 2;
    }
    factor = (sin((-M_PI / 2.0) + M_PI * l->rc) + 1.0) / 2.0;
    l->r = l->ro + (l->rt - l->ro) * factor;
    l->rc += 0.01;
  }
  if (scale) {
    float factor;
    if (l->sc >= 1) {
      /* Reset counter, obtain new target value */
      l->sc = 0;
      l->so = l->st;
      l->st = myrandom(2) - 1;
    }
    factor = (sin((-M_PI / 2.0) + M_PI * l->sc) + 1.0) / 2.0;
    /* Take average of old target and new target, using factor to *
     * weight. It's computed sinusoidally, resulting in smooth,   *
     * rhythmic transitions.                                      */
    l->s = l->so + (l->st - l->so) * factor;
    l->sc += 0.01;
  }
  if (translate) {
    l->txa += myrandom(0.004) - 0.002;
    l->tya += myrandom(0.004) - 0.002;
    l->tx += l->txa;
    l->ty += l->tya;
    if (l->tx > 6) l->txa -= 0.004;
    if (l->ty > 6) l->tya -= 0.004;
    if (l->tx < -6) l->txa += 0.004;
    if (l->ty < -6) l->tya += 0.004;
    if (l->txa > 0.05 || l->txa < -0.05) l->txa /= 1.7;
    if (l->tya > 0.05 || l->tya < -0.05) l->tya /= 1.7;
  }
  if (rotate || scale || translate) {
    lensmatrix(l);
  }
}


#define STEPX(l,x,y) (((l)->ua * (x) + (l)->ub * (y) + (l)->utx) >> 10)
#define STEPY(l,x,y) (((l)->uc * (x) + (l)->ud * (y) + (l)->uty) >> 10)
/*#define STEPY(l,x,y) (((l)->ua * (y) - (l)->ub * (x) + (l)->uty) >> 10)*/

/* Calls itself <lensnum> times - with results from each lens/function.  *
 * After <length> calls to itself, it stops iterating and draws a point. */
void
IFS::recurse(int x, int y, int length, int p)
{
  int i;
  Lens *l;

  if (length == 0) {
    if (p == 0)
      sp(x, y);
    else {
      l = &lenses[p];
      sp(STEPX(l, x, y), STEPY(l, x, y));
    }
  }
  else {
    for (i = 0; i < lensnum; i++) {
      l = &lenses[i];
      recurse(STEPX(l, x, y), STEPY(l, x, y), length - 1, p);
    }
  }
}

/* Performs <count> random lens transformations, drawing a point at each
 * iteration after the first 10.
 */
void
IFS::iterate(int count, int p)
{
  int i;
  Lens *l;
  int x = x;
  int y = y;
  int tx;

# define STEP()                              \
    l = &lenses[random() % lensnum]; \
    tx = STEPX(l, x, y);                     \
    y = STEPY(l, x, y);                      \
    x = tx

  for (i = 0; i < 10; i++) {
    STEP();
  }

  for ( ; i < count; i++) {
    STEP();
    if (p == 0)
      sp(x, y);
    else
      {
	l = &lenses[p];
	sp(STEPX(l, x, y), STEPY(l, x, y));
      }
  }

# undef STEP

  x = x;
  y = y;
}

/* Come on and iterate, iterate, iterate and sing... *
 * Yeah, this function just calls iterate, mutate,   *
 * and then draws everything.                        */
bool IFS::render(uint32_t *p)
{
  int i;
  int partcolor, x, y;

  ccolour++;
  ccolour %= ncolours;

  /* calculate and draw points for this frame */
  x = easel->w << 7;
  y = easel->h << 7;

  clear();

  if (multi) {
    for (i = 0; i < lensnum; i++) {
      partcolor = ccolour * (i+1);
      partcolor %= ncolours;

      auto c = (*pal)(partcolor);
      current_color = 0xff000000 |
              c.getRed().getValue() << 0 |
              c.getGreen().getValue() << 8 |
              c.getBlue().getValue() << 16;


      if (brecurse)
        recurse(x, y, length - 1, i);
      else
        iterate(pow(lensnum, length - 1), i);
    }
  }
  else {

    if (brecurse)
      recurse(x, y, length, 0);
    else
      iterate(pow(lensnum, length), 0);
  }

  for(i = 0; i < lensnum; i++) {
    mutate(&lenses[i]);
  }

  return false;
}


bool IFS::render_gui() {
    bool up = false;

    up |= ScrollableSliderInt("ncolours", &ncolours, 1, 1024*4, "%d", 32);
    ScrollableSliderInt("Detail", &length, 0, 256, "%d", 1);
    //up |= ScrollableSliderInt("Mode", &mode, 0, 32, "%d", 1);
    up |= ImGui::Checkbox("rotate", &rotate);
    up |= ImGui::Checkbox("ifs scale", &scale);
    ImGui::Checkbox("translate", &translate);
    ImGui::Checkbox("recurse", &brecurse);
    ImGui::Checkbox("multi", &multi);

    up |= ScrollableSliderInt("Functions", &lensnum, 1, 32, "%d", 1);
    up |= ImGui::ColorEdit4("Foreground", (float*)&foreground);
    up |= pal.RenderGui();

    if (up) {
        resize(easel->w, easel->h);
    }

    return up;
}

void IFS::resize(int _w, int _h) {
  int i;

  clear();

  pscale = 1;
  if (easel->w > 2560 || easel->h > 2560)
    pscale *= 3;  /* Retina displays */
  /* We aren't increasing the spacing between the pixels, just the size. */

  lenses.clear();
  lenses.resize(lensnum);
  for (i = 0; i < lensnum; i++) {
    CreateLens(
	       myrandom(1)-0.5,
	       myrandom(1),
	       myrandom(4)-2,
	       myrandom(4)+2,
	       &lenses[i]);
  }
  *pal = (*pal).rescale(0., ncolours);
  current_color = ImGui::GetColorU32(foreground);

  widthb = ((easel->w + 31) >> 5);
  width8 = easel->w << 8;
  height8 = easel->h << 8;
}
