#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"
#include <string>


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
    std::string about() const override
    {
        return "Vermiculate is Tyler Pierce's 2001 thread-drawing program, preserved among the XScreenSaver "
               "sources. The original comment points to an FDM project page for the complete program and "
               "documentation. Pavel Vasilyev's Dear ImGui port is dated May 2023. Its changing pictures "
               "emerge from moving colored threads, scripted behavior, and interactions with previously "
               "drawn pixels.\n\n"
               "Each thread has a floating-point position, an integer heading measured in degrees, a color, "
               "and a circular record of recent positions. One movement step adds cos(heading) to x and "
               "sin(heading) to y. Coordinates wrap around the image edges. The heading can follow several "
               "rule families: bounded random turns, occasional fixed-angle turns, steady circles, varying "
               "spiral turns, alternating bends, intermittent curved sections, or a repeated sequence of "
               "turn commands.\n\n"
               "Some configurations steer toward another thread or toward a recorded tail position. The "
               "desired direction is derived from the relative coordinates, then approached with a bounded "
               "angular turn. Other configurations restrict following to horizontal and vertical "
               "directions. These local rules produce pursuit, circling, weaving, and wandering without a "
               "predefined global shape.\n\n"
               "The image also serves as a collision map. Before accepting a step, a thread examines the "
               "color at its proposed destination. Depending on its flags, a collision can reflect its "
               "heading, rotate it by a quarter turn, reverse it, or reject the movement. Threads can "
               "interact with their own trails, other trails, and generated grid walls. Some behaviors "
               "erase wall sections. Old positions are erased or recolored as the circular history "
               "advances, and threads trapped without movement can trigger a reset.\n\n"
               "The archived program includes a compact command language for configuring these behaviors. "
               "This port selects from stored command strings, so many detailed rules are preset rather "
               "than individually exposed as sliders. Cycles controls the number of movement rounds "
               "performed per frame; even zero still performs the first round. Ticks sets the reset "
               "threshold within a frame's movement batch. The current counter starts again each frame, so "
               "a high Ticks setting should not be interpreted as a lifetime in frames. Reset starts a "
               "fresh drawing with the preset's rules, colors, grid, and thread placement.\n\n"
               "Further reading:\n"
               "https://en.wikipedia.org/wiki/Turtle_graphics\n"
               "https://en.wikipedia.org/wiki/Random_walk\n"
               "https://en.wikipedia.org/wiki/XScreenSaver";
    }

    Vermiculate()
        : Art("Vermiculate") {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

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

