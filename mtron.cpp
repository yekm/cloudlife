#include <cstdint>
#include <deque>
#include <bitset>
#include <iostream>

#include <math.h> // exp

#include "imgui.h"
#include "imgui_elements.h"
#include "mtron.hpp"


void Minskytron::resize(int _w, int _h) {
    easel->set_texture_size(1 << tex_power, 1 << tex_power);

    ya=0;               xa=0737777<<ICM;
    yb=060000<<ICM;     xb=0;
    yc=0;               xc=020000<<ICM;

    std::bitset<30> t(tb);
    std::cout << t << std::endl;

    auto mask = ~(~unsigned(0) << SVBW);
    sh0 = ((tb >> SVBW*5) & mask) + CSA;
    sh1 = ((tb >> SVBW*4) & mask) + CSA;
    sh2 = ((tb >> SVBW*3) & mask) + CSA;
    sh3 = ((tb >> SVBW*2) & mask) + CSA;
    sh4 = ((tb >> SVBW*1) & mask) + CSA;
    sh5 = ((tb >> SVBW*0) & mask) + CSA;
    osc.clear();
}


void Minskytron::dt(uint32_t *p, int x, int y, double o, uint32_t c) {
    // keep 10 bits to wrap around 1024 screen pixels
#define SB (32-10)
    x = (x >> (32 - tex_power)) + easel->w/2;
    y = (y >> (32 - tex_power)) + easel->h/2;

    drawdot(p, x, y, o, c);
}

bool Minskytron::render(uint32_t *p) {
    memset(p, 0, easel->w * easel->h * 4);

    for (int i = 0; i<cycles; ++i) {
        ya += (xa + xb) >> sh0;
        xa -= (ya - yb) >> sh1;

        yb += (xb - xc) >> sh2;
        xb -= (yb - yc) >> sh3;

        yc += (xc - xa) >> sh4;
        xc -= (yc - ya) >> sh5;

        osc.emplace_back(odot{xa, ya, xb, yb, xc, yc});
    }

    if (osc.size() > maxodots)
        osc.erase(osc.begin(), osc.end() - maxodots);

    const double N = osc.size();
    double i = 0;

    for (auto & oi : osc) {
        double o = (1-std::exp(-i*gm/N))/(1-std::exp(-gm));
        if (dots_clamped > 0) o /= 2;
        if (i > N - dots_clamped) o = 1;
        if (o<0) o=0;

        dt(p, oi.ax, oi.ay, o, ImGui::GetColorU32(ocolor1));
        dt(p, oi.bx, oi.by, o, ImGui::GetColorU32(ocolor2));
        dt(p, oi.cx, oi.cy, o, ImGui::GetColorU32(ocolor3));
        ++i;
    }

    return true;
}

bool Minskytron::render_gui() {
    bool up = false;

    up |= ScrollableSliderInt("Texture power", &tex_power, 1, 16, "%d", 1);
    ScrollableSliderInt("Max dots", &maxodots, 1024, 1024*16, "%d", 256);
    ScrollableSliderInt("Cycles", &cycles, 0, 4096, "%d", 8);
    ScrollableSliderInt("Dots clamped gamma", &dots_clamped, 0, maxodots, "%d", 8);
    ScrollableSliderFloat("Gamma", &gm, -8, 8, "%.2f", 0.2);

    up |= BitField("Test word", &tb, 0);

    ImGui::ColorEdit3("osc1 color", (float*)&ocolor1);
    ImGui::ColorEdit3("osc2 color", (float*)&ocolor2);
    ImGui::ColorEdit3("osc3 color", (float*)&ocolor3);

    return up;
}
