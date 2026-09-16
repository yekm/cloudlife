#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"

#include <vector>
#include <memory>
#include <deque>
#include <string>


class Minskytron : public Art {
public:
    std::string about() const override
    {
        return "Minskytron traces its origin to Marvin Minsky's early-1960s Tri-Pos display demonstration "
               "on the DEC PDP-1. This attribution is recorded in the repository README and in the linked "
               "PDP-1 reconstruction. The related circle algorithm appears as Minsky's item 149 in MIT's "
               "HAKMEM, AI Memo 239, dated 29 February 1972. Cloudlife implements coupled integer "
               "oscillators directly rather than emulating the original machine.\n\n"
               "A basic shift-and-add oscillator repeatedly changes its coordinates using additions and "
               "subtractions of small fractions of the other coordinate. When those fractions are powers of "
               "two, right shifts replace multiplication. Updating one coordinate and then using its new "
               "value in the other update can create approximately elliptical motion. Minskytron connects "
               "three such coordinate pairs so that each oscillator also responds to the others.\n\n"
               "In this implementation a cycle updates the six integer coordinates sequentially:\n"
               "ya += (xa + xb) >> s0; xa -= (ya - yb) >> s1;\n"
               "yb += (xb - xc) >> s2; xb -= (yb - yc) >> s3;\n"
               "yc += (xc - xa) >> s4; xc -= (yc - ya) >> s5.\n"
               "The six shifts control coupling strength and asymmetry. Their values come from six five-bit "
               "groups of the Test word, each increased by one. This differs from the original PDP-1's "
               "smaller test-word groups, so the linked emulator provides historical context rather than "
               "identical numerical presets.\n\n"
               "After each cycle the three positions are appended to a bounded history. The renderer "
               "redraws that history with separate oscillator colors and an opacity curve based on relative "
               "age. Max dots controls history length, and Cycles controls the number of new steps per "
               "frame. Gamma changes the opacity profile; Dots clamped gamma makes the newest portion fully "
               "bright while reducing the older portion's opacity.\n\n"
               "Texture power sets a square image whose side length is 2 raised to the selected value, and "
               "the oscillator coordinates are projected from their high bits onto this image. Changing the "
               "Test word or texture size resets the oscillators. Try coordinated changes to pairs of shift "
               "groups for different scales, then asymmetric changes for distorted loops and interwoven "
               "motion. Osc1, osc2, and osc3 color distinguish the three paths.\n\n"
               "Further reading:\n"
               "https://www.masswerk.at/minskytron/\n"
               "https://www.inwap.com/pdp10/hbaker/hakmem/hacks.html#item149\n"
               "https://en.wikipedia.org/wiki/PDP-1";
    }

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

