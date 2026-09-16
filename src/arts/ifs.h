#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"
#include <string>

struct Lens {
  float r, s, tx, ty;   /* Rotation, Scale, Translation X & Y */
  float ro, rt, rc;     /* Old Rotation, Rotation Target, Rotation Counter */
  float so, st, sc;     /* Old Scale, Scale Target, Scale Counter */
  float sa, txa, tya;   /* Scale change, Translation change */

  int ua, ub, utx;      /* Precomputed combined r,s,t values */
  int uc, ud, uty;      /* Precomputed combined r,s,t values */

};

class IFS : public Art {
public:
    std::string about() const override
    {
        return "IFS animates fractals formed by repeated affine transformations. Cloudlife uses the xscreensaver "
               "ifs.c lineage preserved in arχiv/xscreensaver/hacks/ifs.c. Its comments credit an earlier hack by "
               "Massimino Pascal as the inspiration, Chris Le Sueur's version in February 2005, substantial "
               "improvements by Robby Griffin in March 2006, and multicolored mode by Jack Grahl in January 2007. "
               "The copyright names Le Sueur and Griffin, 2005-2006. README.md records these credits; Cloudlife's "
               "source additionally credits Pavel Vasilyev's Dear ImGui port in May 2023.\n"
               "\n"
               "Each function, called a lens in the implementation, transforms a point as p' = s*R(r)*p+t: rotation "
               "by angle r, multiplication by scale s, and translation. The actual translations also account for "
               "screen centering. Matrix coefficients are precomputed as integers with an extra factor of 1024; "
               "coordinates use 256 units per pixel, allowing fixed-point multiply-and-shift evaluation. Repeated "
               "contraction and repositioning can create self-similar branches because each transformed copy "
               "contains smaller copies governed by the same rules.\n"
               "\n"
               "There are two sampling methods. With recurse disabled, a chaos-game walk chooses a uniformly random "
               "lens at every step, discards ten initial transformations, and plots later states. Detail sets an "
               "exponential workload based on Functions raised to Detail, capped at 1,048,576 iterations per walk in "
               "this implementation. With recurse enabled, every lens is applied at every recursion level and only "
               "the final points are drawn. This enumerates transformation sequences rather than randomly sampling "
               "them, and its cost grows exponentially without the random-walk cap; large Detail and Functions "
               "values can therefore be extremely expensive. In the current implementation, recursive "
               "multicolored mode requires Detail of at least one; zero gives a negative recursion depth "
               "and can overflow the stack.\n"
               "\n"
               "Rotate and ifs scale enable smooth parameter mutation between randomly selected targets, with "
               "sinusoidal interpolation. Translate gives each lens a gently changing velocity, nudged back toward a "
               "bounded area and damped when too fast. Negative scale targets can invert the transformed copies. "
               "Disable these changes to study a fixed set of functions. The image is cleared and regenerated each "
               "frame before the lenses mutate for the next frame.\n"
               "\n"
               "Multi draws separate colored contributions for the functions, optionally applying one extra lens to "
               "a sampled or recursively generated point. Ncolours controls palette cycling; Foreground provides the "
               "single-color starting value. Functions changes the number of lenses and reinitializes their "
               "parameters. Detail controls sampling density or recursion depth, making it a computational control "
               "as well as a visual one.\n"
               "\n"
               "Further reading:\n"
               "https://en.wikipedia.org/wiki/Iterated_function_system\n"
               "https://en.wikipedia.org/wiki/Chaos_game\n"
               "https://en.wikipedia.org/wiki/Affine_transformation";
    }

    IFS()
        : Art("IFS") {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    int ncolours = 1024;
    int ccolour;

    int widthb;
    int width8, height8;
    int pscale;

    int lensnum = 3;
    std::vector<Lens> lenses;
    int length = 9;
    int mode = 0;
    bool brecurse = false;
    bool multi = true;
    bool translate = true, scale = true, rotate = true;

    ImVec4 foreground = ImVec4(0, 1, 0, 1);
    uint32_t current_color;


    //void drawpoints();
    void sp(int x, int y);
    void lensmatrix(Lens *l);
    void CreateLens(float nr, float ns, float nx, float ny, Lens *newlens);
    void mutate(Lens *l);
    void recurse(int x, int y, int length, int p);
    void iterate(int count, int p);
};

