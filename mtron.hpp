#include "art.hpp"
#include <colormap/colormap.hpp>
#include "imgui.h"

#include "settings.hpp"

#include <vector>
#include <memory>
#include <deque>


class Minskytron : public Art {
public:
    Minskytron()
        : Art("Minskytron") {}

private:
    virtual bool render_gui() override;
    //virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    virtual void resize(int _w, int _h) override;
    void dt(uint32_t *p, int x, int y, double o, uint32_t c);

    struct odot {
        int ax, ay, bx, by, cx, cy;
    };

    std::deque<odot> osc;

    int maxodots = 1024*6;
    int filler_sleep = 100;
    int cycles = 64;
    //float gm = -2.5;
    float gm = 3.5;
    int dots_clamped = 512;
    ImVec4 ocolor1 = ImVec4(1, 1, 0, 0);
    ImVec4 ocolor2 = ImVec4(0, 1, 0, 0);
    ImVec4 ocolor3 = ImVec4(0, 1, 1, 0);
    int tex_power = 10;

    //unsigned int tb = 0b011000111001110011100010000010;
    unsigned int tb = 0b001100011100111001110001000001; // original
    //unsigned int tb = 0b001110011100111001110001000001;
    //unsigned int tb = 0b000110001100111001110001000010; // alt1
    //unsigned int tb = 0b001100010100011001110001100001;
    //                    ....|....|....|....|....|....|

    int ya, xa, yb, xb, yc, xc;

    int sh0, sh1, sh2, sh3, sh4, sh5;

    // constatnt shift add
    static constexpr int CSA = 1;
    // initial constant multiplier
    static constexpr  int ICM = 10;
    // shift value bit width
    static constexpr int SVBW = 5;

    PaletteSetting pal;

};

