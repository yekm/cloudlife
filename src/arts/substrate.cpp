/*
 *  Substrate (dragorn@kismetwireless.net)
 *  Directly ported code from complexification.net Substrate art
 *  http://complexification.net/gallery/machines/substrate/applet_s/substrate_s.pde
 *
 *  Substrate code:
 *  j.tarbell   June, 2004
 *  Albuquerque, New Mexico
 *  complexification.net
 *
 *  CHANGES
 *
 *  1.1  dragorn  Jan 04 2005    Fixed some indenting, typo in errors for parsing
 *                                cmdline args
 *  1.1  dagraz   Jan 04 2005    Added option for circular cracks (David Agraz)
 *                               Cleaned up issues with timeouts in start_crack (DA)
 *  1.0  dragorn  Oct 10 2004    First port done
 *
 * Directly based the hacks of: 
 * 
 * xscreensaver, Copyright (c) 1997, 1998, 2002 Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#include "substrate.hpp"
#include "easelplane.h"
#include "random.h"

#include "imgui.h"

#include <math.h>
#include <cstdlib>
#include <vector>

// Keep the imported random calls and algorithm local to this translation unit.
#define random() LRAND()
#define frand(n) (LRAND() / MAXRAND * (n))

namespace {

/* this program goes faster if some functions are inline.  The following is
 * borrowed from ifs.c */
#if !defined( __GNUC__ ) && !defined(__cplusplus) && !defined(c_plusplus)
#undef inline
#define inline			/* */
#endif

#define STEP 0.42

/* Raw colormap extracted from pollockEFF.gif */
static const char *rgb_colormap[] = {
    "#201F21", "#262C2E", "#352626", "#372B27",
    "#302C2E", "#392B2D", "#323229", "#3F3229",
    "#38322E", "#2E333D", "#333A3D", "#473329",
    "#40392C", "#40392E", "#47402C", "#47402E",
    "#4E402C", "#4F402E", "#4E4738", "#584037",
    "#65472D", "#6D5D3D", "#745530", "#755532",
    "#745D32", "#746433", "#7C6C36", "#523152",
    "#444842", "#4C5647", "#655D45", "#6D5D44",
    "#6C5D4E", "#746C43", "#7C6C42", "#7C6C4B",
    "#6B734B", "#73734B", "#7B7B4A", "#6B6C55",
    "#696D5E", "#7B6C5D", "#6B7353", "#6A745D",
    "#727B52", "#7B7B52", "#57746E", "#687466",
    "#9C542B", "#9D5432", "#9D5B35", "#936B36",
    "#AA7330", "#C45A27", "#D95223", "#D85A20",
    "#DB5A23", "#E57037", "#836C4B", "#8C6B4B",
    "#82735C", "#937352", "#817B63", "#817B6D",
    "#927B63", "#D9893B", "#E49832", "#DFA133",
    "#E5A037", "#F0AB3B", "#8A8A59", "#B29A58",
    "#89826B", "#9A8262", "#888B7C", "#909A7A",
    "#A28262", "#A18A69", "#A99968", "#99A160",
    "#99A168", "#CA8148", "#EB8D43", "#C29160",
    "#C29168", "#D1A977", "#C9B97F", "#F0E27B",
    "#9F928B", "#C0B999", "#E6B88F", "#C8C187",
    "#E0C886", "#F2CC85", "#F5DA83", "#ECDE9D",
    "#F5D294", "#F5DA94", "#F4E784", "#F4E18A",
    "#F4E193", "#E7D8A7", "#F1D4A5", "#F1DCA5",
    "#F4DBAD", "#F1DCAE", "#F4DBB5", "#F5DBBD",
    "#F4E2AD", "#F5E9AD", "#F4E3BE", "#F5EABE",
    "#F7F0B6", "#D9D1C1", "#E0D0C0", "#E7D8C0",
    "#F1DDC6", "#E8E1C0", "#F3EDC7", "#F6ECCE",
    "#F8F2C7", "#EFEFD0", 0
};

typedef struct {
    /* Synthesis of data from Crack:: and SandPainter:: */
    float x, y;
    float t;
    float ys, xs, t_inc; /* for curvature calculations */

    int curved;

    unsigned long sandcolor;
    float sandp, sandg;

    float degrees_drawn;

    int crack_num;

} crack;

struct field {
    int height;
    int width;

    unsigned int initial_cracks;
    
    unsigned int num;
    unsigned int max_num;

    int grains; /* number of grains in the sand painting */

    int circle_percent;

    std::vector<crack> cracks; /* grid of cracks */
    std::vector<int> cgrid; /* grid of actual crack placement */

    /* color parms */
    int numcolors;
    std::vector<uint32_t> parsedcolors;
    unsigned long fgcolor;
    unsigned long bgcolor;
    int visdepth;

    unsigned int cycles;

    unsigned int wireframe;
    unsigned int seamless;
};

struct state {
  EaselPlane *plane = nullptr;
  struct field f{};
};

/* Quick reference to pixels in the crack grid */
#define ref_cgrid(f, x, y)   ((f)->cgrid[(y) * (f)->width + (x)])

static inline void start_crack(struct field *f, crack *cr) 
{
    /* synthesis of Crack::findStart() and crack::startCrack() */
    int px = 0;
    int py = 0;
    int found = 0;
    int timeout = 0;
    float a;

    /* shift until crack is found */
    while ((!found) && (timeout++ < 10000)) {
        px = (int) (random() % f->width);
        py = (int) (random() % f->height);

        if (ref_cgrid(f, px, py) < 10000)
            found = 1;
    }

    if ( !found ) {
        /* We timed out.  Use our default values */
        px = cr->x;
        py = cr->y;

        /* Sanity check needed */
        if (px < 0) px = 0;
        if (px >= f->width) px = f->width - 1;
        if (py < 0) py = 0;
        if (py >= f->height) py = f->height - 1;

        ref_cgrid(f, px, py) = cr->t;
    }

    /* start a crack */
    a = ref_cgrid(f, px, py);

    if ((random() % 100) < 50) {
        /* conversion of the java int(random(-2, 2.1)) */
        a -= 90 + (frand(4.1) - 2.0);
    } else {
        a += 90 + (frand(4.1) - 2.0);
    }

    if ((random() % 100) < f->circle_percent) {
        float r; /* radius */
        float radian_inc;

        cr->curved = 1;
        cr->degrees_drawn = 0;

        r = 10 + (random() % ((f->width + f->height) / 2));

        if ((random() % 100) < 50) {
            r *= -1;
        }

        /* arc length = r * theta => theta = arc length / r */
        radian_inc = STEP / r;
        cr->t_inc = radian_inc * 360 / 2 / M_PI;

        cr->ys = r * sin(radian_inc);
        cr->xs = r * ( 1 - cos(radian_inc));

    }
    else {
        cr->curved = 0;
    }

    /* Condensed from Crack::startCrack */
    cr->x = px + ((float) 0.61 * cos(a * M_PI / 180));
    cr->y = py + ((float) 0.61 * sin(a * M_PI / 180));
    cr->t = a;

}

static inline void make_crack(struct field *f) 
{
    crack *cr;

    if (f->num < f->max_num) {
        /* make a new crack */
        f->cracks.emplace_back();

        cr = &(f->cracks[f->num]);
        /* assign colors */
        cr->sandp = 0;
        cr->sandg = (frand(0.2) - 0.01);
        cr->sandcolor = f->parsedcolors[random() % f->numcolors];
        cr->crack_num = f->num;
        cr->curved = 0;
        cr->degrees_drawn = 0;

        /* We could use these values in the timeout case of start_crack */

        cr->x = random() % f->width;
        cr->y = random() % f->height;
        cr->t = random() % 360;

        /* start it */
        start_crack(f, cr);

        f->num++;
    }
}

// EaselPlane uses packed RGBA bytes, independently of the X11 visual depth.
static inline void point2rgb(int, unsigned long c, int *r, int *g, int *b)
{
    *r = (c >> IM_COL32_R_SHIFT) & 255;
    *g = (c >> IM_COL32_G_SHIFT) & 255;
    *b = (c >> IM_COL32_B_SHIFT) & 255;
}

static inline unsigned long rgb2point(int, int r, int g, int b)
{
    return IM_COL32(r, g, b, 255);
}

// Blend against EaselPlane's current CPU pixel; the caller draws the result.
static inline unsigned long
trans_point(struct state *st,
            int x1, int y1, unsigned long myc, float a, 
            struct field *f) 
{
    if ((x1 >= 0) && (x1 < f->width) && (y1 >= 0) && (y1 < f->height)) {
        if (a >= 1.0) {
            return myc;
        } else {
            int old_r = 0, og = 0, ob = 0;
            int r = 0, g = 0, b = 0;
            int nr, ng, nb;
            unsigned long c;

            c = st->plane->read_pixel(x1, y1);

            point2rgb(f->visdepth, c, &old_r, &og, &ob);
            point2rgb(f->visdepth, myc, &r, &g, &b);

            nr = old_r + (r - old_r) * a;
            ng = og + (g - og) * a;
            nb = ob + (b - ob) * a;

            c = rgb2point(f->visdepth, nr, ng, nb);

            return c;
        }
    }

    return f->bgcolor;
}

static inline void 
region_color(struct state *st, struct field *f, crack *cr) 
{
    /* synthesis of Crack::regionColor() and SandPainter::render() */

    float rx = cr->x;
    float ry = cr->y;
    int openspace = 1;
    int cx, cy;
    float maxg;
    int grains, i;
    float w;
    float drawx, drawy;
    unsigned long c;

    // On a seamless image a ray can wrap forever without meeting a crack.
    int remaining = 2 * (f->width + f->height);
    while (openspace && remaining-- > 0) {
        /* move perpendicular to crack */
        rx += (0.81 * sin(cr->t * M_PI/180));
        ry -= (0.81 * cos(cr->t * M_PI/180));

        cx = (int) rx;
        cy = (int) ry;
        if (f->seamless) {
            cx = (cx % f->width + f->width) % f->width;
            cy = (cy % f->height + f->height) % f->height;
        }

        if ((cx >= 0) && (cx < f->width) && (cy >= 0) && (cy < f->height)) {
            /* safe to check */
            if (f->cgrid[cy * f->width + cx] > 10000) {
                /* space is open */
            } else {
                openspace = 0;
            }
        } else {
            openspace = 0;
        }
    }

    /* SandPainter stuff here */

    /* Modulate gain */
    cr->sandg += (frand(0.1) - 0.050);
    maxg = 1.0;

    if (cr->sandg < 0)
        cr->sandg = 0;

    if (cr->sandg > maxg)
        cr->sandg = maxg;

    grains = f->grains;

    /* Lay down grains of sand */
    w = cr->sandg / (grains - 1);

    for (i = 0; i < grains; i++) {
        drawx = (cr->x + (rx - cr->x) * sin(cr->sandp + sin((float) i * w)));
        drawy = (cr->y + (ry - cr->y) * sin(cr->sandp + sin((float) i * w)));
        if (f->seamless) {
            drawx = fmod(fmod(drawx, f->width) + f->width, f->width);
            drawy = fmod(fmod(drawy, f->height) + f->height, f->height);
        }

        /* Draw sand bit */
        c = trans_point(st, drawx, drawy, cr->sandcolor, (0.1 - i / (grains * 10.0)), f);

        st->plane->drawdot((int) drawx, (int) drawy, c);
    }
}

static void build_substrate(struct field *f) 
{
    unsigned int tx;
    /* int ty; */

    f->cycles = 0;

    f->cracks.clear();
    f->cracks.reserve(f->max_num);
    f->num = 0;

    /* erase the crack grid */
    f->cgrid.assign(static_cast<size_t>(f->height) * f->width, 10001);

    /* Not necessary now that make_crack ensures we have usable default
     *  values in start_crack's timeout case 
    * make random crack seeds *
    for (tx = 0; tx < 16; tx++) {
        ty = (int) (random() % (f->width * f->height - 1));
        f->cgrid[ty] = (int) random() % 360;
    }
    */

    /* make the initial cracks */
    for (tx = 0; tx < f->initial_cracks; tx++)
        make_crack(f);
}


static inline void
movedrawcrack(struct state *st, struct field *f, int cracknum) 
{
    /* Basically Crack::move() */

    int cx, cy;
    crack *cr = &(f->cracks[cracknum]);

    /* continue cracking */
    if ( !cr->curved ) {
        cr->x += ((float) STEP * cos(cr->t * M_PI/180));
        cr->y += ((float) STEP * sin(cr->t * M_PI/180));
    }
    else {
        cr->x += ((float) cr->ys * cos(cr->t * M_PI/180));
        cr->y += ((float) cr->ys * sin(cr->t * M_PI/180));

        cr->x += ((float) cr->xs * cos(cr->t * M_PI/180 - M_PI / 2));
        cr->y += ((float) cr->xs * sin(cr->t * M_PI/180 - M_PI / 2));

        cr->t += cr->t_inc;
        cr->degrees_drawn += fabsf(cr->t_inc);
    }
    if (f->seamless) {
        cr->x = fmod(cr->x + f->width, f->width);
        cr->y = fmod(cr->y + f->height, f->height);
    }

    /* bounds check */
    /* modification of random(-0.33,0.33) */
    cx = (int) (cr->x + (frand(0.66) - 0.33));
    cy = (int) (cr->y + (frand(0.66) - 0.33));
    if (f->seamless) {
        cx = (cx % f->width + f->width) % f->width;
        cy = (cy % f->height + f->height) % f->height;
    }

    if ((cx >= 0) && (cx < f->width) && (cy >= 0) && (cy < f->height)) {
        /* draw sand painter if we're not wireframe */
        if (!f->wireframe)
            region_color(st, f, cr);

        /* draw fgcolor crack */
        st->plane->drawdot(cx, cy, f->fgcolor);

        if ( cr->curved && (cr->degrees_drawn > 360) ) {
            /* completed the circle, stop cracking */
            start_crack(f, cr); /* restart ourselves */
            make_crack(f); /* generate a new crack */
        }
        /* safe to check */
        else if ((f->cgrid[cy * f->width + cx] > 10000) ||
                 (fabsf(f->cgrid[cy * f->width + cx] - cr->t) < 5)) {
            /* continue cracking */
            f->cgrid[cy * f->width + cx] = (int) cr->t;
        } else if (fabsf(f->cgrid[cy * f->width + cx] - cr->t) > 2) {
            /* crack encountered (not self), stop cracking */
            start_crack(f, cr); /* restart ourselves */
            make_crack(f); /* generate a new crack */
        }
    } else {
        /* out of bounds, stop cracking */

	/* need these in case of timeout in start_crack */
        cr->x = random() % f->width;
        cr->y = random() % f->height;
        cr->t = random() % 360;

        start_crack(f, cr); /* restart ourselves */
        make_crack(f); /* generate a new crack */
    }

}


} // namespace

#undef random
#undef frand
#undef STEP
#undef ref_cgrid

struct Substrate::State : state {
    State()
    {
        auto* f = &this->f;
        f->fgcolor = IM_COL32(0, 0, 0, 255);
        f->bgcolor = IM_COL32(255, 255, 255, 255);
        f->visdepth = 32;
        while (rgb_colormap[f->numcolors] != nullptr) {
            const unsigned long rgb = strtoul(rgb_colormap[f->numcolors] + 1, nullptr, 16);
            f->parsedcolors.push_back(rgb2point(32,
                (rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255));
            f->numcolors++;
        }
    }

};

Substrate::Substrate() : Art("Substrate"), m_state(std::make_unique<State>())
{
    usePlane();
    m_state->plane = eplane();
}

Substrate::~Substrate() = default;

void Substrate::resize(int width, int height)
{
    default_resize(width, height);
    m_state->f.width = std::max(0, width);
    m_state->f.height = std::max(0, height);
    restart();
}

void Substrate::restart()
{
    auto* f = &m_state->f;
    if (f->width <= 0 || f->height <= 0) return;
    f->initial_cracks = m_initial_cracks;
    f->max_num = m_max_cracks;
    f->grains = m_grains;
    f->circle_percent = m_circle_percent;
    f->wireframe = m_wireframe;
    f->seamless = m_seamless;
    build_substrate(f);
    for (int y = 0; y < f->height; ++y)
        for (int x = 0; x < f->width; ++x)
            drawdot(x, y, f->bgcolor);
    m_next_frame = 0;
}

bool Substrate::render(uint32_t*)
{
    auto* f = &m_state->f;
    if (m_paused || f->width <= 0 || f->height <= 0 || ImGui::GetTime() < m_next_frame)
        return false;
    // Preserve the original loop: cracks born this cycle also advance this cycle.
    for (unsigned tempx = 0; tempx < f->num; tempx++)
        movedrawcrack(m_state.get(), f, tempx);
    f->cycles++;
    if (m_max_cycles != 0 && f->cycles >= (unsigned) m_max_cycles)
        restart();
    m_next_frame = ImGui::GetTime() + m_growth_delay / 1000000.0;
    return false;
}

bool Substrate::render_gui()
{
    const auto tooltip = [](const char* text) {
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 32.0f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    };
    auto* f = &m_state->f;
    ImGui::SliderInt("Initial cracks (next restart)", &m_initial_cracks, 3, 15);
    tooltip("The number of moving crack tips used to seed a fresh drawing. Each tip traces a line or arc, "
            "then starts another path when it reaches a boundary or another crack. More initial tips "
            "start growth in more places at once; fewer let a small number of structures develop first.\n\n"
            "Applies on the next restart and does not change the current drawing.");
    if (ImGui::SliderInt("Maximum cracks", &m_max_cracks, 16, 500)) {
        f->max_num = m_max_cracks;
        // Retire excess moving tips without erasing their cracks or shading.
        f->num = std::min(f->num, f->max_num);
        f->cracks.resize(f->num);
        f->cracks.reserve(f->max_num);
    }
    tooltip("Limits the number of crack tips that can grow simultaneously, not the number of lines already "
            "on the paper. Collisions, edge encounters, and completed circles can spawn additional tips "
            "until this limit is reached. Higher limits allow more simultaneous growth and require more work per cycle.\n\n"
            "Lowering the limit immediately retires excess tips but keeps their marks. Raising it allows "
            "future spawning rather than creating all the extra tips at once.");
    ImGui::SliderInt("Sand grains", &m_grains, 16, 128);
    tooltip("The number of translucent color samples deposited by each moving tip during a growth step. "
            "Samples spread sideways from the crack toward the next boundary, building the soft, sandy "
            "shading between lines. More grains generally produce denser, smoother shading; fewer leave "
            "a sparser, grainier texture and cost less to draw.\n\n"
            "Changes affect new deposits immediately. Existing shading stays as painted. "
            "This setting has no visible effect while Wireframe only is enabled.");
    ImGui::SliderInt("Circle percentage", &m_circle_percent, 0, 100);
    tooltip("The probability, in percent, that a newly started crack follows a circular arc instead of "
            "a straight line. At 0, all new paths are straight; at 100, all are curved. Arc radius and "
            "turn direction are chosen randomly. Cracks may collide before completing a circle, so "
            "this is not the percentage of complete circles in the image.\n\n"
            "Applies when a tip starts or restarts a path. Existing paths keep their current shape.");
    ImGui::Checkbox("Wireframe only", &m_wireframe);
    tooltip("Draws only the dark crack lines, without depositing colored sand beside them. This reveals "
            "the branching and collision structure and reduces the work needed for shading.\n\n"
            "Takes effect immediately, but previously painted color remains. Restart with this enabled "
            "for a clean line-only drawing; turn it off to resume sand painting along the moving tips.");
    ImGui::Checkbox("Seamless", &m_seamless);
    tooltip("Connects opposite edges of the canvas: tips and sand deposits that cross one edge wrap "
            "around to the other. With this disabled, the edges stop paths and cause tips to start "
            "again elsewhere. Wrapping lets structures continue across the canvas boundaries.\n\n"
            "Changes future movement immediately. Existing edge marks are not repaired or redrawn; "
            "enable this before restarting to use wrapping throughout the drawing.");
    f->grains = m_grains;
    f->circle_percent = m_circle_percent;
    f->wireframe = m_wireframe;
    f->seamless = m_seamless;
    if (ImGui::SliderInt("Growth delay (us)", &m_growth_delay, 0, 100000))
        m_next_frame = 0;
    tooltip("The minimum pause between growth cycles, in microseconds (1,000 us = 1 ms). Each cycle "
            "advances the moving tips and deposits their sand. A larger delay slows the drawing; zero "
            "allows a cycle on every rendered frame. Actual speed also depends on rendering and simulation cost.\n\n"
            "Changes pacing immediately without changing the distance a tip moves per cycle or clearing the image.");
    ImGui::SliderInt("Maximum cycles (0 = unlimited)", &m_max_cycles, 0, 25000);
    tooltip("How many growth cycles a drawing runs before automatically clearing the paper and starting "
            "a new composition. Larger values give cracks and shading more time to accumulate. This is "
            "a cycle count, not a duration in seconds; Growth delay and rendering speed affect its duration.\n\n"
            "Zero disables automatic restarting. Lowering this below the current cycle count triggers "
            "a restart after the next growth cycle. Pausing stops the counter.");
    ImGui::Checkbox("Pause", &m_paused);
    tooltip("Stops tip movement, sand deposits, and the cycle counter while keeping the current picture "
            "visible. Resume to continue from the same paths.\n\n"
            "You can adjust settings while paused; their effects appear when growth resumes. "
            "Restart still clears and reseeds the drawing while paused.");
    if (ImGui::Button("Restart")) restart();
    tooltip("Clears all cracks and shading back to white paper, resets the cycle counter, and creates "
            "a fresh random composition using the current settings, including Initial cracks. "
            "The previous drawing is discarded.\n\n"
            "Settings and pause state are preserved. The shared Shuffle button also starts a fresh drawing.");
    ImGui::Text("Cracks: %u / %d   Cycles: %u", m_state->f.num, m_max_cracks, m_state->f.cycles);
    ImGui::TextWrapped("Uses the original Pollock palette on white paper. Live changes preserve existing marks. "
                       "Initial cracks applies on the next restart.");
    return false;
}

void Substrate::shuffle()
{
    restart();
}

std::string Substrate::about() const
{
    return "Substrate by Jared Tarbell (2004), ported to XScreenSaver by Mike Kershaw, "
           "with circular cracks by David Agraz.\n\n"
           "Cracks start approximately perpendicular to existing cracks, grow until they meet "
           "another crack or the image boundary, and spawn new cracks. Curved cracks trace arcs. "
           "A sand painter deposits translucent grains between each crack and the next boundary, "
           "using the original Pollock-derived palette on white.\n\n"
           "Initial and maximum cracks control population; sand grains controls shading density; "
           "circle percentage chooses curved growth. Seamless wraps the image boundaries. "
           "Sand grains, wireframe, seamless, and the crack limit change live, preserving existing marks. "
           "Lowering the crack limit retires excess moving tips; raising it permits further spawning. "
           "Circle percentage affects new paths, while initial cracks applies on the next restart. "
           "Growth delay controls pacing, and maximum cycles controls regeneration (zero disables it). "
           "Shuffle and Restart begin a fresh drawing; Pause preserves the current image.\n\n"
           "http://complexification.net/gallery/machines/substrate/\n"
           "https://www.jwz.org/xscreensaver/";
}
