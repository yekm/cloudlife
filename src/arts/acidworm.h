#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"

typedef struct
{
    int orientation, head;
    //short *xpos, *ypos;
    std::vector<short> xpos, ypos;
} worm_t;

class AcidWorm : public Art {
public:
    AcidWorm()
        : Art("AcidWorm") {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    int length = 100;
    int number = 256;
    int color_cycle = 1; /* Normal color cycling     */
    int position = 1;    /* Centered worm placement  */

    std::vector<worm_t> worm;
    std::vector<short *> ref;
    std::vector<char> mpp;
    std::vector<short> _ip;
    short *ip;


    int max_x = 0, max_y = 0;

    int last = 0, bottom = 0;

    void gl_setpixel(int x, int y, int ci);
};

