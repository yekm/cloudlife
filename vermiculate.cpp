/*
 *  @(#) vermiculate.c
 *  @(#) Copyright (C) 2001 Tyler Pierce (tyler@alumni.brown.edu)
 *  The full program, with documentation, is available at:
 *    http://freshmeat.net/projects/fdm
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * Dear ImGui port by Pavel Vasilyev <yekm@299792458.ru>, May 2023
 */


#include <ctype.h>
#include <math.h>

#include "imgui_elements.h"
#include "random.h"
#include "vermiculate.h"

#define degs2 (degs/2)
#define degs4 (degs/4)
#define degs8 (degs/8)
#define dtor 0.0174532925        /*  pi / degs2; */
#define tailmax (thrmax * 2 + 1)
#define tmodes '7'
#define ymax (h - 1)
#define ymin 0
#define xmax (w - 1)
#define xmin 0
#define SPEEDINC 10
#define SPEEDMAX 1000
#define wraparound(VAL,LOWER,UPPER) {         \
                    if (VAL >= UPPER)        \
                      VAL -= UPPER - LOWER;        \
                    else if (VAL < LOWER)        \
                      VAL += UPPER - LOWER; }
#define arrcpy(DEST,SRC) memcpy (DEST, SRC, sizeof(DEST))

static const struct stringAndSpeed
{
  const char * const str;
  int speed;
}
sampleStrings[] =
{
/*  { "]]]]]]]]7ces123400-9-8#c123456#s9998880004#ma3#car9ma6#c-#r1", 600} ,
        { "bmarrrr#c1234#lc5678#lyet]", 600} , */
  { "AEBMN222222223#CAR9CAD4CAOV", 150} ,
  { "mn333#c23#f1#]]]]]]]]]]]3bc9#r9#c78#f9#ma4#", 600} ,
  { "AEBMN22222#CAD4CAORc1#f2#c1#r6", 100} ,
/*  { "mn6666666#c1i#f1#y2#sy2#vyety1#ry13i#l", 40} , */
  { "aebmnrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr#", 500} ,
/*  { "bg+++++++++++++++++++++++#mnrrrrrrrrrrrrrrrrrrrrrrr#y1#k", 500},  */
/*  { "BMN22222223#CAD4CAOVYAS", 150}, */
/*  { "aebmnrrrrrrrrrrrrrrrr#yaryakg--#", 100} , */
  { "mn6rrrrrrrrrrrrrrr#by1i#lcalc1#fnyav" , 200 } ,
  { "mn1rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr#by1i#lcalc1#fn", 2000 },
  { "baeMn333333333333333333333333#CerrYerCal", 800 },
  { "baeMn1111111111111111111111111111111111111111111111111111111111#Cer9YesYevYerCal", 1200 },
  { "baMn111111222222333333444444555555#Ct1#lCt2#lCt3#lCt4#lCt5#lCerrYerYet", 1400 },
  { "baMn111111222222333333444444555555#Ct1#lCt2#lCt3#lCt4#lCt5#lCerrYerYetYt1i#lYt1i#sYt1#v", 1400 }
};




bool Vermiculate::wasakeypressed ()
{
  if (*instring != 0)
    return true;
  else
    return false;
}

char Vermiculate::readkey ()
{
  char readkey_result;
  if (*instring == 0)
    {
      readkey_result = '#';
    }
  else
    {
      readkey_result = *instring;
      instring++;
    };
  return toupper (readkey_result);
}

unsigned int Vermiculate::random1 (unsigned int i)
{
  return (xoshiro256plus () % i);
}

unsigned long Vermiculate::waitabit ()
{
  int result = 0;
  cyc += threads;
  while (cyc > speed)
    {
      result += 10000;
      cyc -= speed;
    }
  return result;
}

void Vermiculate::clearscreen ()
{
  clear();

  fill0(point);
}

void
Vermiculate::sp (int x, int y, uint32_t c)
{
  uint32_t color = pal.get_color(c);
  if (c == 0)
    color = 0;
  drawdot(x, y, color);

  point[(w * y) + x] = c;
}

int
Vermiculate::gp (int x, int y)
{
  return point[(w * y) + x];
}

void
Vermiculate::redraw (int x, int y, int width, int height)
{
  int xc, yc;
  for (xc = x; xc <= x + width - 1; xc++)
    for (yc = y; yc <= y + height - 1; yc++)
      if (point[w * yc + xc] != 0)
        sp (xc, yc, point[w * yc + xc]);
}

void
Vermiculate::palupdate (bool forceUpdate)
{
  if (forceUpdate || *instring == 0)
    {
      redraw (xmin, ymin, w, h);
    }
}

void
Vermiculate::randpal ()
{
  int ncolors = tailmax - 1;
  pal.rescale(ncolors);
}

void
Vermiculate::gridupdate (bool interruptible)
{
  int x, y;
  if (gridden > 0)
    for (x = 0; x <= xmax && !(wasakeypressed () && interruptible); x += boxw)
      for (y = 0; y <= ymax; y += boxh)
        {
          if (random1 (15) < gridden)
            {
#define lesser(A,B) ( ((A)<(B)) ? (A) : (B) )
              int max = lesser (x + boxw, xmax);
              int xc;
              for (xc = x; xc <= max; xc++)
                sp (xc, y, 1);
            }
          if (random1 (15) < gridden)
            {
              int max = lesser (y + boxh, ymax);
              int yc;
              for (yc = y; yc <= max; yc++)
                sp (x, yc, 1);
            }
        }
}

void
Vermiculate::bordupdate ()
{
  int xbord, ybord;

  if (bordcorn == 0 || bordcorn == 1)
    ybord = ymin;
  else
    ybord = ymax;
  if (bordcorn == 0 || bordcorn == 3)
    xbord = xmin;
  else
    xbord = xmax;
  {
    int x, y;
    for (x = xmin; x <= xmax; x++)
      sp (x, ybord, bordcol);
    for (y = ymin; y <= ymax; y++)
      sp (xbord, y, bordcol);
  }
}

bool
Vermiculate::inbank (unsigned char thr)
{
  int c;
  if (bnkt > 0)
    for (c = 1; c <= bnkt; c++)
      if (bank[c - 1] == thr)
        return true;
  return false;
}

void
Vermiculate::pickbank ()
{
  unsigned char thr = 1;
  bnkt = 0;
  ch = '\0';
  do
    {
      while (inbank (thr))
        thr = thr % threads + 1;

      palupdate (false);
      ch = readkey ();
      palupdate (false);
      switch (ch)
        {
        case '+':
        case '-':
          do
            {
              if (ch == '+')
                thr++;
              else
                thr--;
              wraparound (thr, 1, threads + 1);
            }
          while (inbank (thr));
          break;
        case ' ':
          bank[++bnkt - 1] = thr;
          break;
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':

          bank[++bnkt - 1] = ch - '0';
          if (bank[bnkt - 1] > threads)
            bnkt--;
          break;
        case 'I':
          {
            banktype tbank;
            int tbankt = 0;
            int c;
            for (c = 1; c <= threads; c++)
              if (!inbank (c))
                tbank[++tbankt - 1] = c;
            bnkt = tbankt;
            arrcpy (bank, tbank);
          }
          break;
        case 'T':
          ch = readkey ();
          switch (ch)
            {
            case '1': case '2': case '3':
            case '4': case '5': case '6':
            case '7': case '8': case '9':

              {
                int c;
                for (c = 1; c <= threads; c++)
                  if (thread[c - 1].tmode == ch - '0')
                    bank[++bnkt - 1] = c;
              }
              break;
            }
          break;
        case 'A':
          for (bnkt = 1; bnkt <= threads; bnkt++)
            bank[bnkt - 1] = bnkt;
          bnkt = threads;
          break;
        case 'E':
          for (bnkt = 1; bnkt <= thrmax; bnkt++)
            bank[bnkt - 1] = bnkt;
          bnkt = thrmax;
          break;
        }
    }
  while (!(bnkt >= threads || ch == 'N' || ch == '\15' || ch == '#'));
  if (bnkt == 0 && ch != 'N')
    {
      bnkt = 1;
      bank[0] = thr;
    }
  palupdate (false);
}

void
Vermiculate::bankmod (char boolop, bool * Bool_)
{
  switch (boolop)
    {
    case 'T':
      *Bool_ = !*Bool_;
      break;
    case 'Y':
      *Bool_ = true;
      break;
    case 'N':
      *Bool_ = false;
      break;
    }
}

void
Vermiculate::newonscreen (unsigned char thr)
{
  linedata *LP = &thread[thr - 1];
  LP->filled = false;
  LP->dead = false;
  LP->reclen = (LP->little) ?
        random1 (10) + 5 : random1 (rlmax - 30) + 30;
  LP->deg = random1 (degs);
  LP->y = random1 (h);
  LP->x = random1 (w);
  LP->recpos = 0;
  LP->turnco = 2;
  LP->turnsize = random1 (4) + 2;
}

void
Vermiculate::firstinit (unsigned char thr)
{
  linedata *LP = &thread[thr - 1];
  LP->col = thr + 1;
  LP->prey = 0;
  LP->tmode = 1;
  LP->slice = degs / 3;
  LP->orichar = 'R';
  LP->spiturn = 5;
  LP->selfbounce = false;
  LP->realbounce = false;
  LP->vhfollow = false;
  LP->tailfollow = false;
  LP->killwalls = false;
  LP->little = false;
  LP->ctinc = random1 (2) * 2 - 1;
  LP->circturn = ((thr % 2) * 2 - 1) * ((thr - 1) % 7 + 1);
  LP->tsc = 1;
  LP->tslen = 6;
  LP->turnseq[0] = 6;
  LP->turnseq[1] = -6;
  LP->turnseq[2] = 6;
  LP->turnseq[3] = 6;
  LP->turnseq[4] = -6;
  LP->turnseq[5] = 6;
  LP->tclim = (unsigned char) (((real) degs) / 2 / 12);
}

void
Vermiculate::maininit ()
{
  if (!instring)
    {
      int n = random1 (sizeof (sampleStrings) / sizeof (sampleStrings[0]));
      if (oinstring) free (oinstring);
      instring = oinstring = strdup (sampleStrings[n].str);
      speed = sampleStrings[n].speed;
    }
  boxh = 10;
  boxw = 10;
  gridden = 0;
  bordcorn = 0;
  threads = 4;
  curviness = 30;
  bordcol = 1;
  ogd = 8;
  ch = '\0';
  erasing = true;
  {
    unsigned char thr;
    for (thr = 1; thr <= thrmax; thr++)
      {
        firstinit (thr);
        newonscreen (thr);
      }
  }
  {
    int d;
    for (d = degs - 1; d >= 0; d--)
      {
        sinof[d] = sin (d * dtor);
        cosof[d] = cos (d * dtor);
        if (d % degs4 == 0)
          tanof[d] = tanof[d + 1];
        else
          tanof[d] = tan (d * dtor);
      }
  }
  randpal ();
}

bool Vermiculate::move (unsigned char thr)
{
  linedata *LP = &thread[thr - 1];
  if (LP->dead)
    return (false);
  if (LP->prey == 0)
    switch (LP->tmode)
      {
      case 1:
        LP->deg += random1 (2 * LP->turnsize + 1) - LP->turnsize;
        break;
      case 2:
        if (LP->slice == degs || LP->slice == degs2 || LP->slice == degs4)
          {
            if (LP->orichar == 'D')
              {
                if (LP->deg % degs4 != degs8)
                  LP->deg = degs4 * random1 (4) + degs8;
              }
            else if (LP->orichar == 'V')
              if (LP->deg % degs4 != 0)
                LP->deg = degs4 * random1 (4);
          }
        if (random1 (100) == 0)
          {
            if (LP->slice == 0)
              LP->deg = LP->deg - degs4 + random1 (degs2);
            else
              LP->deg += (random1 (2) * 2 - 1) * LP->slice;
          }
        break;
      case 3:
        LP->deg += LP->circturn;
        break;
      case 4:
        if (abs (LP->spiturn) > 11)
          LP->spiturn = 5;
        else
          LP->deg += LP->spiturn;
        if (random1 (15 - abs (LP->spiturn)) == 0)
          {
            LP->spiturn += LP->ctinc;
            if (abs (LP->spiturn) > 10)
              LP->ctinc *= -1;
          }
        break;
      case 5:
        LP->turnco = abs (LP->turnco) - 1;
        if (LP->turnco == 0)
          {
            LP->turnco = curviness + random1 (10);
            LP->circturn *= -1;
          }
        LP->deg += LP->circturn;
        break;
      case 6:
        if (abs (LP->turnco) == 1)
          LP->turnco *= -1 * (random1 (degs2 / abs (LP->circturn)) + 5);
        else if (LP->turnco == 0)
          LP->turnco = 2;
        else if (LP->turnco > 0)
          {
            LP->turnco--;
            LP->deg += LP->circturn;
          }
        else
          LP->turnco++;
        break;
      case 7:
        LP->turnco++;
        if (LP->turnco > LP->tclim)
          {
            LP->turnco = 1;
            LP->tsc = (LP->tsc % LP->tslen) + 1;
          }
        LP->deg += LP->turnseq[LP->tsc - 1];
        break;
      }
  else
    {
      int desdeg;
      real dy, dx;
      if (LP->tailfollow || LP->prey == thr)
        {
          dx = thread[LP->prey - 1].xrec[thread[LP->prey - 1].recpos] - LP->x;
          dy = thread[LP->prey - 1].yrec[thread[LP->prey - 1].recpos] - LP->y;
        }
      else
        {
          dx = thread[LP->prey - 1].x - LP->x;
          dy = thread[LP->prey - 1].y - LP->y;
        }
      desdeg =
        (LP->vhfollow) ?
        ((fabs (dx) > fabs (dy)) ?
         ((dx > 0) ?
          0 * degs4
          :
          2 * degs4)
         :
         ((dy > 0) ?
          1 * degs4
          :
          3 * degs4))
        :
        ((dx > 0) ?
         ((dy > 0) ?
          1 * degs8 : 7 * degs8) : ((dy > 0) ? 3 * degs8 : 5 * degs8));
      if (desdeg - desdeg % degs4 != LP->deg - LP->deg % degs4
          || LP->vhfollow)
        {
          if (!LP->vhfollow)
           {
              /* Using atan2 here doesn't seem to slow things down: */
              desdeg = atan2 (dy, dx) / dtor;
              wraparound (desdeg, 0, degs);
           }
          if (abs (desdeg - LP->deg) <= abs (LP->circturn))
            LP->deg = desdeg;
          else
            LP->deg +=
              (desdeg > LP->deg) ?
              ((desdeg - LP->deg > degs2) ?
               -abs (LP->circturn) : abs (LP->circturn))
              : ((LP->deg - desdeg > degs2) ?
                 abs (LP->circturn) : -abs (LP->circturn));
        }
      else
        LP->deg +=
          (tanof[LP->deg] >
           dy / dx) ? -abs (LP->circturn) : abs (LP->circturn);
    }

  wraparound (LP->deg, 0, degs);
  {
    unsigned char oldcol;
    real oldy = LP->y, oldx = LP->x;
    LP->x += cosof[LP->deg];
    wraparound (LP->x, xmin, xmax + 1);
    LP->y += sinof[LP->deg];
    wraparound (LP->y, ymin, ymax + 1);
#define xi ((int) LP->x)
#define yi ((int) LP->y)

    oldcol = gp (xi, yi);
    if (oldcol != 0)
      {
        bool vertwall = false, horiwall = false;
        if (oldcol == 1 && ((LP->killwalls && gridden > 0) || LP->realbounce))
          {
            vertwall = (gp (xi, (int) oldy) == 1);
            horiwall = (gp ((int) oldx, yi) == 1);
          }
        if (oldcol == 1 && LP->realbounce && (vertwall || horiwall))
          {
            if (vertwall)
              LP->deg = -LP->deg + degs2;
            else
              LP->deg = -LP->deg;
          }
        else
          {
            if ((oldcol != LP->col && LP->realbounce)
                || (oldcol == LP->col && LP->selfbounce))
              LP->deg += degs4 * (random1 (2) * 2 - 1);
            else if (oldcol != LP->col)
              LP->deg += degs2;
          }
        if (LP->killwalls && gridden > 0 && oldcol == 1)
          {
            if (vertwall && xi + 1 <= xmax)
              {
                int yy;
                for (yy = yi - yi % boxh;
                     yy <= yi - yi % boxh + boxh && yy <= ymax; yy++)
                  if (gp (xi + 1, yy) != 1 || yy == ymax)
                    sp (xi, yy, 0);
              }
            if (horiwall && yi + 1 <= ymax)
              {
                int xx;
                for (xx = xi - xi % boxw;
                     xx <= xi - xi % boxw + boxw && xx <= xmax; xx++)
                  if (gp (xx, yi + 1) != 1 || xx == xmax)
                    sp (xx, yi, 0);
              }
          }
        if (oldcol != LP->col || LP->selfbounce)
          {
            LP->x = oldx;
            LP->y = oldy;
          }
        wraparound (LP->deg, 0, degs);
      }
  }

  sp (xi, yi, LP->col);
  if (LP->filled)
    {
      if (erasing)
        sp (LP->xrec[LP->recpos], LP->yrec[LP->recpos], 0);
      else
        sp (LP->xrec[LP->recpos], LP->yrec[LP->recpos], LP->col + thrmax);
    }
  LP->yrec[LP->recpos] = yi;
  LP->xrec[LP->recpos] = xi;
  if (LP->recpos == LP->reclen - 1)
    LP->filled = true;
  if (LP->filled && !erasing)
    {
      int co = LP->recpos;
      LP->dead = true;
      do
        {
          int nextco = co + 1;
          wraparound (nextco, 0, LP->reclen);
          if (LP->yrec[co] != LP->yrec[nextco]
              || LP->xrec[co] != LP->xrec[nextco])
            LP->dead = false;
          co = nextco;
        }
      while (!(!LP->dead || co == LP->recpos));
    }
  LP->recpos++;
  wraparound (LP->recpos, 0, LP->reclen);
  return (!LP->dead);
}

bool Vermiculate::render (uint32_t *p)
{
  int had_instring = (instring != 0);
  int tick = 0;
  int this_delay = 0;
  int loop = 0;


 AGAIN:
  if (reset_p)
    {
      reset_p = 0;

      clearscreen ();
      {
        unsigned char thr;
        for (thr = 1; thr <= threads; thr++)
          newonscreen (thr);
      }
      if (autopal)
        {
          randpal ();
          palupdate (false);
        }
      bordupdate ();
      gridupdate (false);
    }

  {
    bool alltrap = true;
    unsigned char thr;
    for (thr = 1; thr <= threads; thr++)
      if (move (thr))
        alltrap = false;
    if (alltrap)        /* all threads are trapped */
      reset_p = true;
    //if (speed != SPEEDMAX)
    //  this_delay = waitabit ();
  }

  if (tick++ > max_ticks && !had_instring)
    {
      tick = 0;
      if (oinstring) free (oinstring);
      instring = oinstring = 0;
      maininit();
      reset_p = true;
      autopal = false;
    }

  //if (this_delay == 0 && loop++ < 1000)
  if (loop++ < cycles)
    goto AGAIN;

  return false;
}

bool Vermiculate::render_gui ()
{
    bool up = false;

    ScrollableSliderInt("ticks", &max_ticks, 0, 20000, "%d", 8);
    ScrollableSliderInt("cycles", &cycles, 0, 1024, "%d", 1);
    //up |= ScrollableSliderInt("Speed", &speed, 1, 1024, "%d", 1);
    up |= pal.RenderGui();

	// TODO: select instring

    if (up) {
        resize(w, h);
    }

    return up;
}


void Vermiculate::resize (int _w, int _h)
{
  default_resize(_w, _h);
  point.resize(w*h);
  fill0(point);
  reset_p = 1;
  instring = 0;
  maininit ();
  palupdate (true);
  consume_instring();
}

void Vermiculate::consume_instring()
{
  char boolop;

  while (wasakeypressed ())
    {
      ch = readkey ();
      switch (ch)
        {
        case 'M':
          ch = readkey ();
          switch (ch)
            {
            case 'A':
            case 'N':
              {
                unsigned char othreads = threads;
                if (ch == 'N')
                  threads = 0;
                do
                  {
                    ch = readkey ();
                    switch (ch)
                      {
                      case '1': case '2': case '3':
                      case '4': case '5': case '6':
                      case '7': case '8': case '9':
                        thread[++threads - 1].tmode = ch - '0';
                        break;
                      case 'R':
                        thread[++threads - 1].tmode =
                          random1 (tmodes - '0') + 1;
                        break;
                      }
                  }
                while (!(ch == '\15' || ch == '#'
                         || threads == thrmax));
                if (threads == 0)
                  threads = othreads;
                reset_p = 1;
              }
              break;
            }
          break;
        case 'C':
          pickbank ();
          if (bnkt > 0)
            {
              ch = readkey ();
              switch (ch)
                {
                case 'D':
                  ch = readkey ();
                  switch (ch)
                    {
                    case '1': case '2': case '3':
                    case '4': case '5': case '6':
                    case '7': case '8': case '9':
        /* Careful!  The following macro needs to be at the beginning of any
        block in which it's invoked, since it declares variables: */
        #define forallinbank(LDP) linedata *LDP; int bankc; \
        for (bankc = 1;        \
        ((bankc <= bnkt) ? ( \
                (LDP = &thread[bank[bankc - 1] - 1],        1) \
                ) : 0) ; bankc++)
                      {
                        forallinbank (L) L->slice = degs / (ch - '0');
                      }
                      break;
                    case 'M':
                      {
                        forallinbank (L) L->slice = 0;
                      }
                      break;
                    }
                  break;
                case 'S':
                  {
                    forallinbank (L)
                    {
                      L->otslen = L->tslen;
                      L->tslen = 0;
                    }
                  }
                  do
                    {
                      char oldch = ch;
                      ch = readkey ();
                      {
                        forallinbank (L)
                        {
                          switch (ch)
                            {
                            case '0':
                            case '1': case '2': case '3':
                            case '4': case '5': case '6':
                            case '7': case '8': case '9':
                              L->tslen++;
                              L->turnseq[L->tslen - 1] = ch - '0';
                              if (oldch == '-')
                                L->turnseq[L->tslen - 1] *= -1;
                              if (bankc % 2 == 0)
                                L->turnseq[L->tslen - 1] *= -1;
                              break;
                            }
                        }
                      }
                    }
                  while (!(ch == '\15' || ch == '#'
                           || thread[bank[0] - 1].tslen == 50));
                  {
                    forallinbank (L)
                    {
                      int seqSum = 0, c;
                      if (L->tslen == 0)
                        L->tslen = L->otslen;
                      for (c = 1; c <= L->tslen; c++)
                        seqSum += L->turnseq[c - 1];
                      if (seqSum == 0)
                        L->tclim = 1;
                      else
                        L->tclim =
                          (int) (((real) degs2) / abs (seqSum));
                      L->tsc = random1 (L->tslen) + 1;
                    }
                  }
                  break;
                case 'T':
                  {
                    ch = readkey ();
                    {
                      forallinbank (L)
                      {
                        switch (ch)
                          {
                          case '1': case '2': case '3':
                          case '4': case '5': case '6':
                          case '7': case '8': case '9':
                            L->tmode = ch - '0';
                            break;
                          case 'R':
                            L->tmode = random1 (tmodes - '0') + 1;
                            break;
                          }
                      }
                    }
                  }
                  break;
                case 'O':
                  ch = readkey ();
                  {
                    forallinbank (L) L->orichar = ch;
                  }
                  break;
                case 'F':
                  {
                    banktype fbank;
                    arrcpy (fbank, bank);
                    {
                      int fbnkt = bnkt;
                      int bankc;
                      pickbank ();
                      for (bankc = 1; bankc <= fbnkt; bankc++)
                        {
                          linedata *L = &thread[fbank[bankc - 1] - 1];
                          if (ch == 'N')
                            L->prey = 0;
                          else
                            L->prey = bank[0 + (bankc - 1) % bnkt];
                        }
                    }
                  }
                  break;
                case 'L':
                  {
                    forallinbank (L) L->prey = bank[bankc % bnkt];
                  }
                  break;
                case 'R':
                  ch = readkey ();
                  {
                    forallinbank (L) switch (ch)
                      {
                      case '1': case '2': case '3':
                      case '4': case '5': case '6':
                      case '7': case '8': case '9':
                        L->circturn = 10 - (ch - '0');
                        break;
                      case 'R':
                        L->circturn = random1 (7) + 1;
                        break;
                      }
                  }
                  break;
                }
            }
          break;
        case 'T':
        case 'Y':
        case 'N':
          boolop = ch;
          pickbank ();
          if (bnkt > 0)
            {
              ch = readkey ();
              {
                forallinbank (L)
                {
                  switch (ch)
                    {
                    case 'S':
                      bankmod (boolop, &L->selfbounce);
                      break;
                    case 'V':
                      bankmod (boolop, &L->vhfollow);
                      break;
                    case 'R':
                      bankmod (boolop, &L->realbounce);
                      break;
                    case 'L':
                      bankmod (boolop, &L->little);
                      cleared = true;
                      break;
                    case 'T':
                      bankmod (boolop, &L->tailfollow);
                      break;
                    case 'K':
                      bankmod (boolop, &L->killwalls);
                      break;
                    }
                }
              }
            }
          break;
        case 'R':
          if (bordcol == 1)
            {
              bordcol = 0;
              bordupdate ();
              bordcorn = (bordcorn + 1) % 4;
              bordcol = 1;
              bordupdate ();
            }
          break;
        case '1': case '2': case '3':
        case '4': case '5': case '6':
        case '7': case '8': case '9':
          {
            int c;
            for (c = 1; c <= thrmax; c++)
              thread[c - 1].tmode = ch - '0';
          }
          break;
        case '\40':
          cleared = true;
          break;
        case 'E':
          erasing = !erasing;
          break;
        case 'P':
          randpal ();
          palupdate (true);
          break;
        case 'G':
          {
            char dimch = 'B';
            bool gridchanged = true;
            if (gridden == 0)
              gridden = ogd;
            do
              {
                int msize = 0;
                if (gridchanged)
                  {
                    clearscreen ();
                    gridupdate (true);
                  }
                ch = readkey ();
                gridchanged = true;
                switch (ch)
                  {
                  case '+':
                    msize = 1;
                    break;
                  case '-':
                    msize = -1;
                    break;
                  case ']':
                    if (gridden < 15)
                      gridden++;
                    break;
                  case '[':
                    if (gridden > 0)
                      gridden--;
                    break;
                  case 'O':
                    ogd = gridden;
                    gridden = 0;
                    break;
                  case 'S':
                    boxw = boxh;
                  case 'W':
                  case 'H':
                  case 'B':
                    dimch = ch;
                    break;
                  default:
                    gridchanged = false;
                  }
                if (dimch == 'W' || dimch == 'B')
                  boxw += msize;
                if (dimch == 'H' || dimch == 'B')
                  boxh += msize;
                if (boxw == 0)
                  boxw = 1;
                if (boxh == 0)
                  boxh = 1;
              }
            while (!(ch == '\15' || ch == '#' || ch == 'O'));
            cleared = true;
          }
          break;
        case 'A':
          autopal = !autopal;
          break;
        case 'B':
          bordcol = 1 - bordcol;
          bordupdate ();
          break;
        case '-':
          speed -= SPEEDINC;
          if (speed < 1)
            speed = 1;
          break;
        case '+':
          speed += SPEEDINC;
          if (speed > SPEEDMAX)
            speed = SPEEDMAX;
          break;
        case '/':
          if (curviness > 5)
            curviness -= 5;
          break;
        case '*':
          if (curviness < 50)
            curviness += 5;
          break;
        case ']':
          if (threads < thrmax)
            newonscreen (++threads);
          break;
        case '[':
          if (threads > 1)
            {
              linedata *L = &thread[threads - 1];
              int lastpos = (L->filled) ? L->reclen - 1 : L->recpos;
              int c;
              for (c = 0; c <= lastpos; c++)
                sp (L->xrec[c], L->yrec[c], 0);
              threads--;
            }
          break;
        }
    }
}
