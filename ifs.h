#include "art.hpp"
#include "imgui.h"
#include <vector>

#include "settings.hpp"

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
    IFS()
        : Art("IFS") {}
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual void render(uint32_t *p) override;
private:

    int ncolours = 1024;
    int ccolour;

    int widthb;
    int width8, height8;
    //std::vector<unsigned int> board;
    int pscale;

    int lensnum = 3;
    std::vector<Lens> lenses;
    int length = 9;
    int mode = 0;
    bool brecurse = false;
    bool multi = true;
    bool translate = true, scale = true, rotate = true;

    ImVec4 foreground = ImVec4(0, 1, 0, 1);
    PaletteSetting pal;
    uint32_t current_color;


    //void drawpoints();
    void sp(int x, int y);
    void lensmatrix(Lens *l);
    void CreateLens(float nr, float ns, float nx, float ny, Lens *newlens);
    void mutate(Lens *l);
    void recurse(int x, int y, int length, int p);
    void iterate(int count, int p);
};

