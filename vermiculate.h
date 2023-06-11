#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"


#define degs 360
#define thrmax 120
#define rlmax 200

typedef double real;
typedef unsigned char banktype[thrmax];

typedef struct linedata
{
  int deg, spiturn, turnco, turnsize;
  unsigned char col;
  bool dead;

  char orichar;
  real x, y;
  int tmode, tsc, tslen, tclim, otslen, ctinc, reclen, recpos, circturn, prey,
    slice;
  int xrec[rlmax + 1], yrec[rlmax + 1];
  int turnseq[50];
  bool filled, killwalls, vhfollow,
    selfbounce, tailfollow, realbounce, little;
}
linedata;


class Vermiculate : public Art {
public:
    Vermiculate()
        : Art("Vermiculate") {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual void render(uint32_t *p) override;

  int speed = 1;
  bool erasing, cleared, autopal;
  char *oinstring = 0;       /* allocated */
  const char *instring = 0;  /* consumed */
  int max_ticks = 20000;

  real sinof[degs], cosof[degs], tanof[degs];
  std::vector<uint32_t> point;

  linedata thread[thrmax];
  banktype bank;
  int bnkt;
  int boxw, boxh, curviness, gridden, ogd, bordcorn;
  unsigned char bordcol, threads;
  char ch, boolop;

  int reset_p = 1;
  int cyc;
  int cycles = 1;

  PaletteSetting pal;

  bool wasakeypressed();
  char readkey ();
  unsigned int random1 (unsigned int i);
  unsigned long waitabit ();
  void clearscreen ();
  void sp (int x, int y, uint32_t c);
  int gp (int x, int y);
  void redraw (int x, int y, int width, int height);
  void palupdate (bool forceUpdate);
  void randpal ();
  void gridupdate (bool interruptible);
  void bordupdate ();

  bool inbank (unsigned char thr);
  void pickbank ();

  void bankmod (char boolop, bool * Bool_);
  void newonscreen (unsigned char thr);
  void firstinit (unsigned char thr);
  void maininit ();
  bool move (unsigned char thr);
  void consume_instring();


};

