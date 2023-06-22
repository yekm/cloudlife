#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"

/* costs ~6% speed */
#define dither_when_mapped 1

class RDbomb : public Art {
public:
    RDbomb()
        : Art("reaction/diffusion textures") {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    void pixack_init();
    void pixack_frame();
    void random_colors();
    void rd_init();

#if dither_when_mapped
    unsigned char *mc;
#endif
    int ncolors = 65535;
    int iterations = 3;

    int mapped;
    int pdepth;

    std::vector<unsigned short> r1, r2, r1b, r2b;
    int width = 256, height = 256, npix;
    int radius = 8;
    int reaction = 1;
    int diffusion = 1;

    char *pd;
    int array_width, array_height;

    double array_x, array_y;
    double array_dx, array_dy;

    PaletteSetting pal;

};

