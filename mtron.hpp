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
        : Art("Minskytron") { reinit(); }
    virtual bool override_texture_size(int &_w, int &_h) {
        _w = W; _h = H;
        return true;
    }

private:
    virtual bool render_gui() override;
    //virtual void resize(int _w, int _h) override;
    virtual void render(uint32_t *p) override;

    void reinit();
    void dt(uint32_t *p, int x, int y, double o, uint32_t c);

    struct odot {
        int ax, ay, bx, by, cx, cy;
    };

    std::deque<odot> osc;

    int maxodots = 1024*6;
    int filler_sleep = 100;
    int maxdots_perframe = 64;
    //float gm = -2.5;
    float gm = 3.5;
    int dots_clamped = 64;
    ImVec4 ocolor1 = ImVec4(1, 0, 0, 0);
    ImVec4 ocolor2 = ImVec4(0, 1, 0, 0);
    ImVec4 ocolor3 = ImVec4(0, 0, 1, 0);

    int density = 32, cycles=0;
    ImVec4 clear_color = ImVec4(1, 0, 0, 1.00f);
    ImVec4 background = ImVec4(0, 0, 0, 1);
    ImVec4 foreground = ImVec4(0, 1, 0, 1);


    //unsigned int tb = 0b011000111001110011100010000010;
    unsigned int tb = 0b001100011100111001110001000001; // original
    //unsigned int tb = 0b001110011100111001110001000001;
    //unsigned int tb = 0b000110001100111001110001000010; // alt1
    //unsigned int tb = 0b001100010100011001110001100001;
    //                    ....|....|....|....|....|....|

    int ya, xa, yb, xb, yc, xc;

    int sh0, sh1, sh2, sh3, sh4, sh5;

    static constexpr int W = 1024;
    static constexpr int H = 1024;
    static constexpr int TEXTURE_SIZE = W*H*4;
    // constatnt shift add
    static constexpr int CSA = 1;
    // initial constant multiplier
    static constexpr  int ICM = 10;
    // shift value bit width
    static constexpr int SVBW = 5;

    PaletteSetting pal;

};

