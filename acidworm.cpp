
/* Acidworm 0.2 - By Aaron Tiensivu - tiensivu@pilot.msu.edu */
/* Gross hack of Eric P. Scott's 'worm' program              */
/* Inspired by Acidwarp and other eyecandy programs          */

/* If you like it, tell me. If you hate it, tell me.         */
/* If it gives you epileptic seisures, enjoy!                */

#include <stdio.h> 
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <math.h>

#include "imgui_elements.h"
#include "acidworm.h"

#include "random.h"
#define random xoshiro256plus

static unsigned char pal[257 * 3];
 
static struct options {
  int nopts;
  int opts[3];
  }
    normal[8] = {
      { 3, { 7, 0, 1 } },
      { 3, { 0, 1, 2 } },
      { 3, { 1, 2, 3 } },
      { 3, { 2, 3, 4 } },
      { 3, { 3, 4, 5 } },
      { 3, { 4, 5, 6 } },
      { 3, { 5, 6, 7 } },
      { 3, { 6, 7, 0 } }},

    upper[8] = {
      { 1, { 1, 0, 0 } },
      { 2, { 1, 2, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 2, { 4, 5, 0 } },
      { 1, { 5, 0, 0 } },
      { 2, { 1, 5, 0 } }},

    left[8] = {
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 2, { 2, 3, 0 } },
      { 1, { 3, 0, 0 } },
      { 2, { 3, 7, 0 } },
      { 1, { 7, 0, 0 } },
      { 2, { 7, 0, 0 } }},

    right[8] = {
      { 1, { 7, 0, 0 } },
      { 2, { 3, 7, 0 } },
      { 1, { 3, 0, 0 } },
      { 2, { 3, 4, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 2, { 6, 7, 0 } }},

    lower[8] = {
      { 0, { 0, 0, 0 } },
      { 2, { 0, 1, 0 } },
      { 1, { 1, 0, 0 } },
      { 2, { 1, 5, 0 } },
      { 1, { 5, 0, 0 } },
      { 2, { 5, 6, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } }},

    upleft[8] = {
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 1, { 3, 0, 0 } },
      { 2, { 1, 3, 0 } },
      { 1, { 1, 0, 0 } }},

    upright[8] = {
      { 2, { 3, 5, 0 } },
      { 1, { 3, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 1, { 5, 0, 0 } }},

    lowleft[8] = {
      { 3, { 7, 0, 1 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 1, { 1, 0, 0 } },
      { 2, { 1, 7, 0 } },
      { 1, { 7, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } }},

    lowright[8] = {
      { 0, { 0, 0, 0 } },
      { 1, { 7, 0, 0 } },
      { 2, { 5, 7, 0 } },
      { 1, { 5, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } },
      { 0, { 0, 0, 0 } }};

const char *LSD_module = "acidworm";

static short 
  xinc[] = 
    {1,  1,  1,  0, -1, -1, -1,  0},
  yinc[] = 
    {-1,  0,  1,  1,  1,  0, -1, -1};

void setcustompalette(int color_cycle);
void shiftpalette(void);

static uint32_t pal_rgb(int i) {
  if (i >= 256)
    printf("%d ", i);
  return 0xff000000 |
         (pal[i*3+0]*4) << 0 |
         (pal[i*3+1]*4) << 8 |
         (pal[i*3+2]*4) << 16;
}


void AcidWorm::gl_setpixel(int x, int y, int ci) {
  auto c = pal_rgb(ci);
  drawdot(x, y, c);
}

bool AcidWorm::render(uint32_t *) {
	worm_t *w;
	struct options *op;

	int x = 0, y = 0;

	unsigned int tmp1,tmp2;

	int 
	max_color = 0,
	ch = 0,
	h = 0,
	n = 0, 
  //position = 0,
  position2= 0;

  //const char *header = "\nAcidWorm version 0.2 - By Aaron Tiensivu - [12/05/96] - SpunkMunky International\n";

  if (color_cycle)
    shiftpalette();

  for (n = 0, w = &worm[0]; n < number; n++, w++) {

    if ((x = w->xpos[h = w->head]) < 0) {
      if (position < 6)
        position2 = position;
      else
        position2 = (random() % 6);

      switch (position2) {
      case 0:
        tmp1 = ((random() % last) + 1);
        tmp2 = ((random() % bottom) + 1);
        break;
      case 1:
        tmp1 = (last / 2);
        tmp2 = (bottom / 2);
        break;
      case 2:
        tmp1 = tmp2 = 1;
        break;
      case 3:
        tmp1 = last-1;
        tmp2 = 1;
        break;
      case 4:
        tmp1 = 1;
        tmp2 = bottom-1;
        break;
      case 5:
        tmp1 = last - 1;
        tmp2 = bottom - 1;
        break;
      }
      gl_setpixel(x = w->xpos[h] = tmp1, y = w->ypos[h] = tmp2, n%256);
      ref[y][x]++;
    } else
      y = w->ypos[h];

    if (++h == length)
      h = 0;
    if (w->xpos[w->head = h] >= 0) {
      int x1, y1;

      x1 = w->xpos[h];
      y1 = w->ypos[h];
      if (--ref[y1][x1] == 0)
        gl_setpixel(x1, y1, 0);
    }

    op = &(!x ? (!y ? upleft : (y == bottom ? lowleft : left))
              : (x == last
                      ? (!y ? upright : (y == bottom ? lowright : right))
                      : (!y ? upper
                            : (y == bottom ? lower
                                          : normal))))[w->orientation];
    switch (op->nopts) {
    case 1:
      w->orientation = op->opts[0];
      break;
    case 0:
      w->orientation = 0;
      break;
    default:
      //w->orientation = op->opts[random() % op->nopts];
      w->orientation = op->opts[((unsigned)random()) % op->nopts];
    }
    gl_setpixel(x += xinc[w->orientation], y += yinc[w->orientation], n%256);
    ref[w->ypos[h] = y][w->xpos[h] = x]++;
  }

  return false;
}

/* This routine screams 'rewrite me' */
/* Feel free to do so, I'm working on a new one now */

void setcustompalette(int color_cycle)
{
  int i;
  int r1,r2,r3;
  int r4,r5,r6;

  switch (color_cycle)
  {
    case 0: /* No color cycles - RGB palette */
    case 1: /* RGB palette - cycles          */

      for (i = 0; i < 32; ++i)
      {
        pal[ i        * 3    ] = (unsigned char)i * 2;
        pal[(i +  64) * 3    ] = (unsigned char)0;
        pal[(i + 128) * 3    ] = (unsigned char)0;
        pal[(i + 192) * 3    ] = (unsigned char)i * 2;
  
        pal[ i        * 3 + 1] = (unsigned char)0;
        pal[(i +  64) * 3 + 1] = (unsigned char)i * 2;
        pal[(i + 128) * 3 + 1] = (unsigned char)0;
        pal[(i + 192) * 3 + 1] = (unsigned char)i * 2;
  
        pal[ i        * 3 + 2] = (unsigned char)0;
        pal[(i +  64) * 3 + 2] = (unsigned char)0;
        pal[(i + 128) * 3 + 2] = (unsigned char)i * 2;
        pal[(i + 192) * 3 + 2] = (unsigned char)i * 2;
      }

      for (i = 32; i < 64; ++i)
      {
        pal[ i        * 3    ] = (unsigned char)(63 - i) * 2;
        pal[(i +  64) * 3    ] = (unsigned char)0;
        pal[(i + 128) * 3    ] = (unsigned char)0;
        pal[(i + 192) * 3    ] = (unsigned char)(63 - i) * 2;
  
        pal[ i        * 3 + 1] = (unsigned char)0;
        pal[(i +  64) * 3 + 1] = (unsigned char)(63 - i) * 2;
        pal[(i + 128) * 3 + 1] = (unsigned char)0;
        pal[(i + 192) * 3 + 1] = (unsigned char)(63 - i) * 2;
  
        pal[ i        * 3 + 2] = (unsigned char)0;
        pal[(i +  64) * 3 + 2] = (unsigned char)0;
        pal[(i + 128) * 3 + 2] = (unsigned char)(63 - i) * 2;
        pal[(i + 192) * 3 + 2] = (unsigned char)(63 - i) * 2;
      }
      break;

    case 2: /* Monochrome to desert palette */
      for (i = 1; i < 128; ++i)
      {
        pal[ i        * 3    ] = (unsigned char)(16 + (i/4));
        pal[ i        * 3 + 1] = (unsigned char)(16 + (i/4));
        pal[ i        * 3 + 2] = (unsigned char)(16 + (i/4));
  
        pal[(i + 128) * 3    ] = (unsigned char)(16 + ((127 - i)/4));
        pal[(i + 128) * 3 + 1] = (unsigned char)(16 + ((127 - i)/4));
        pal[(i + 128) * 3 + 2] = (unsigned char)(16 + ((127 - i)/4));
      }
      break;

    case 3: /* WavyRave palette - don't tell Moby */
      r1 = ( ( random() % 40 ) + 1);
      r2 = ( ( random() % 40 ) + 1);
      r3 = ( ( random() % 40 ) + 1);
      r4 = ( ( random() % 40 ) + 1);
      r5 = ( ( random() % 40 ) + 1);
      r6 = ( ( random() % 40 ) + 1);

      for (i= 1; i< 128; i++)
      {
        pal[i         * 3]     = (unsigned char) ( cos ( i / r1 ) * sin ( i / r2 ) * 31 + 31);
        pal[i         * 3 + 1] = (unsigned char) ( sin ( i / r2 ) * cos ( i / r3 ) * 31 + 31);
        pal[i         * 3 + 2] = (unsigned char) ( sin ( i / r3 ) * sin ( i / r1 ) * 31 + 31);

        pal[(i + 128) * 3]     = (unsigned char) ( sin ( i / r4 ) * cos ( i / r5 ) * 31 + 31);
        pal[(i + 128) * 3 + 1] = (unsigned char) ( cos ( i / r5 ) * sin ( i / r6 ) * 31 + 31);
        pal[(i + 128) * 3 + 2] = (unsigned char) ( cos ( i / r6 ) * cos ( i / r4 ) * 31 + 31);
      }
      break;

    case 4: /* 'Eugene Jarvis is god' palette - Robotron */
      for (i= 1; i< 256; i++)
      {
        pal[i * 3    ] = (unsigned char) random();
        pal[i * 3 + 1] = (unsigned char) random();
        pal[i * 3 + 2] = (unsigned char) random();
      }
      break;
    default:
      break;
  }
  //gl_setpalette(pal);
}

/* These last two functions are taken from Acidwarp */
void shiftpalette()
{
  int temp;
  int x;
  int color = (int)(random() % 3);

    temp = pal[3+color];
    for(x=1; x < (256) ; ++x)
      pal[x*3+color] = pal[(x*3)+3+color];
    pal[((256)*3)-3+color] = temp;
  //gl_setpalette(pal);
}

bool AcidWorm::render_gui() {
  bool up = false, re = false;

  re |= ScrollableSliderInt("color_cycle", &color_cycle, 0, 4, "%d", 1);
  re |= ScrollableSliderInt("length", &length, 2, 1024, "%d", 1);
  re |= ScrollableSliderInt("number", &number, 0, 1024, "%d", 8);
  re |= ScrollableSliderInt("position", &position, 0, 6, "%d", 1);
  //ImGui::Checkbox("fade_dir", &fade_dir);

  if (re) {
  }

  //pal.RenderGui();

  //ImGui::Text("state:%d frame:%d", acid_state, frame);

  up = re;
  return up;
}

void AcidWorm::resize(int _w, int _h) {
	//worm_t *wrm;

  clear();
  
  last      = (easel->w - 1);
  bottom    = (easel->h - 1);
  max_x = easel->w;
  max_y = easel->h;

  worm.resize(number);
  //mpp.resize(1024);
  _ip.resize(max_x * max_y); ip = _ip.data();
  ref.resize(max_y);
  fill0(ref);

  for (int n = 0; n < max_y; ++n) {
    ref[n] = ip;
    ip += max_x;
  }

  auto wrm = &worm[0];

  for (int n = number; --n >= 0; wrm++) {
    wrm->xpos.resize(length);
    wrm->ypos.resize(length);
    fillN<-1>(wrm->xpos);
    fillN<-1>(wrm->ypos);
  }

  setcustompalette(color_cycle);

}
