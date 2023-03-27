#include <cstdint>
#include <algorithm>

#include "cloudlife.hpp"
#include "imgui_elements.h"
#include "random.h"

void Cloudlife::resize_field(int fw, int fh) {
    int s = fw * fh * sizeof(unsigned char);
    f->width = fw;
    f->height = fh;

    f->cells.resize(s);
    f->new_cells.resize(s);
    std::fill(f->cells.begin(), f->cells.end(), 0);
    std::fill(f->new_cells.begin(), f->new_cells.end(), 0);

    *pal = (*pal).rescale(0., f->max_age);
    //*pal = (*pal).rescale(0., ncolors);
}

unsigned char *Cloudlife::cell_at(unsigned int x, unsigned int y)
{
    return (f->cells.data() + x * sizeof(unsigned char) +
                       y * f->width * sizeof(unsigned char));
}

unsigned char *Cloudlife::new_cell_at(unsigned int x, unsigned int y)
{
    return (f->new_cells.data() + x * sizeof(unsigned char) +
                           y * f->width * sizeof(unsigned char));
}

static unsigned int
random_cell(unsigned int p)
{
    int r = xoshiro256plus() & 0xff;

    if (r < p) {
        return (1);
    } else {
        return (0);
    }
}

void Cloudlife::populate_field(unsigned int p)
{
    unsigned int x, y;

    for (x = 0; x < f->width; x++) {
        for (y = 0; y < f->height; y++) {
            *cell_at(x, y) = random_cell(p);
        }
    }
}

void Cloudlife::resize(int _w, int _h) {
    default_resize(_w, _h);

    if (f->height != h / (1 << f->cell_size) + 2 ||
        f->width != w / (1 << f->cell_size) + 2) {
        refield();
    }
}

void Cloudlife::refield() {
    resize_field(w / (1 << f->cell_size) + 2,
                h / (1 << f->cell_size) + 2);
    populate_field(density);
    default_resize(w, h); // clear pixels
}

bool Cloudlife::render_gui() {
    bool up = false;

    up |= ScrollableSliderInt("Initial density", &density, 8, 256, "%d", 8);
    up |= ScrollableSliderUInt("Cell size", &f->cell_size, 1, 32, "%d", 1);
    up |= ScrollableSliderUInt("Max age", &f->max_age, 4, 256, "%d", 8);
    //up |= ScrollableSliderUInt("ncolors", &ncolors, 0, 1024, "%d", 8);
    ScrollableSliderUInt("Ticks per frame", &f->ticks_per_frame, 1, 128, "%d", 1);
    up |= ImGui::ColorEdit4("Foreground", (float*)&foreground);
    up |= ImGui::ColorEdit4("Backgroud", (float*)&background);
    up |= ImGui::ColorEdit4("Clear color", (float*)&clear_color);
    up |= pal.RenderGui();

    if (up) {
        refield();
    }

    return up;
}


void Cloudlife::populate_edges(unsigned int p)
{
    unsigned int i;

    for (i = f->width; i--;) {
        *cell_at(i, 0) = random_cell(p);
        *cell_at(i, f->height - 1) = random_cell(p);
    }

    for (i = f->height; i--;) {
        *cell_at(f->width - 1, i) = random_cell(p);
        *cell_at(0, i) = random_cell(p);
    }
}

//--------------------------------------------------------------

uint32_t Cloudlife::get_color_age(int age) {
    auto c = (*pal)(age);
    return  0xff000000 |
            c.getRed().getValue() << 0 |
            c.getGreen().getValue() << 8 |
            c.getBlue().getValue() << 16;
}

void
Cloudlife::draw_field()
{
    unsigned int x, y;
    unsigned int rx, ry = 0;	/* random amount to offset the dot */
    unsigned int size = 1 << f->cell_size;
    unsigned int mask = size - 1;
    unsigned int fg_count, bg_count;
    uint32_t fgc, bgc;

    fgc = ImGui::GetColorU32(foreground);
    //fgc = get_color_age(colorindex);// original color behaviour?
    bgc = ImGui::GetColorU32(background);

    /* columns 0 and width-1 are off screen and not drawn. */
    for (y = 1; y < f->height - 1; y++) {
        fg_count = 0;
        bg_count = 0;

        /* rows 0 and height-1 are off screen and not drawn. */
        for (x = 1; x < f->width - 1; x++) {
            rx = xoshiro256plus();
            ry = rx >> f->cell_size;
            rx &= mask;
            ry &= mask;

            uint8_t age = *cell_at(x, y);
            fgc = get_color_age(age);
            if (age) {
                drawdot((short) x *size - rx - 1,
                        (short) y *size - ry - 1,
                        fgc);
            } else {
                drawdot((short) x *size - rx - 1,
                        (short) y *size - ry - 1,
                        bgc);
            }
        }
    }
}

static inline unsigned int
cell_value(unsigned char c, unsigned int age)
{
    if (!c) {
        return 0;
    } else if (c > age) {
        return (3);
    } else {
        return (1);
    }
}

unsigned int Cloudlife::is_alive(unsigned int x, unsigned int y)
{
    unsigned int count;
    unsigned int i, j;
    unsigned char *p;

    count = 0;

    for (i = x - 1; i <= x + 1; i++) {
        for (j = y - 1; j <= y + 1; j++) {
            if (y != j || x != i) {
                count += cell_value(*cell_at(i, j), f->max_age);
            }
        }
    }

    p = cell_at(x, y);
    if (*p) {
        if (count == 2 || count == 3) {
            return ((*p) + 1);
        } else {
            return (0);
        }
    } else {
        if (count == 3) {
            return (1);
        } else {
            return (0);
        }
    }
}

unsigned int Cloudlife::do_tick()
{
    unsigned int x, y;
    unsigned int count = 0;
    for (x = 1; x < f->width - 1; x++) {
        for (y = 1; y < f->height - 1; y++) {
            count += *new_cell_at(x, y) = is_alive(x, y);
        }
    }
    f->cells = f->new_cells;
    return count;
}

void Cloudlife::render(uint32_t *p) {
    unsigned int count = 0;

    for (int i=0; i<f->ticks_per_frame; ++i)
        count = do_tick();

    if (count < (f->height + f->width) / 4) {
        populate_field(density);
    }

    if (cycles % (f->max_age / 2) == 0) {
        populate_edges(density);
        do_tick();
        populate_edges(0);
    }
/*
    if (ncolors) {
        if (colortimer) {
            colorindex--;
            if (colorindex == 0)
                colorindex = ncolors;
        }
    }
*/
    draw_field();
    cycles++;

    std::copy(pixels.begin(), pixels.end(), p);

}


