#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"

#include <vector>
#include <memory>
#include <string>


struct field {
    unsigned int height = 0;
    unsigned int width = 0;
    unsigned int max_age = 64;
    unsigned int cell_size = 3;
    std::vector<uint8_t> cells, new_cells;
};

class Cloudlife : public Art {
public:
    std::string about() const override
    {
        return "Cloudlife was written by Don Marti as a variation on Conway's Game of Life. Its source "
               "records a 20 May 2003 update adding color cycling and a manual page, and credits examples "
               "from XScreenSaver. Pavel Vasilyev's Dear ImGui port is dated January 2023. Marti's defining "
               "change gives cells an age-dependent influence, helping formations evolve instead of "
               "remaining indefinitely static.\n\n"
               "The simulation stores either zero for a dead cell or a positive age for a living cell. To "
               "update an interior cell, it sums the contributions of its eight neighbors. A dead neighbor "
               "contributes zero. A living neighbor at or below Max age contributes one; a living neighbor "
               "older than that threshold contributes three. A living cell survives and increments its age "
               "when the weighted sum is two or three, and dies otherwise. A dead cell becomes age one when "
               "the sum is exactly three.\n\n"
               "For young cells these are the familiar Life survival and birth rules. An old neighbor's "
               "extra weight changes what its surroundings see, disrupting otherwise stable arrangements. "
               "Max age is therefore an influence threshold, rather than an instruction to kill a cell as "
               "soon as it reaches a birthday. The weighted rule can also cause births that ordinary Life "
               "would not permit.\n\n"
               "Rendering samples one randomly offset pixel within every logical cell each tick, using the "
               "cycling palette color for living cells and the background color for dead cells. Over repeated "
               "frames the scattered samples reveal the grid and leave softly changing, cloudlike traces. "
               "Moving formations acquire the cometlike appearance described by the original author. The "
               "simulation also repopulates a depleted field and periodically injects random activity at "
               "its edges.\n\n"
               "Initial density sets the probability of a living starting cell, approximately density/256. "
               "Cell size is an exponent: a logical cell spans 2 raised to that setting in screen pixels, "
               "so increasing it reduces the number of simulated cells. Max age changes how long cells "
               "retain ordinary neighbor weight. Color cycle interval sets the ticks between palette steps; "
               "zero uses the fixed Foreground color. The palette controls set the colormap and number of "
               "colors. Background and clear color also affect appearance. "
               "Changing these settings repopulates the field, providing a fresh initial condition for "
               "comparing rules and scales.\n\n"
               "Further reading:\n"
               "https://en.wikipedia.org/wiki/Conway%27s_Game_of_Life\n"
               "https://en.wikipedia.org/wiki/Cellular_automaton\n"
               "https://en.wikipedia.org/wiki/XScreenSaver";
    }

    Cloudlife()
        : Art("Cloudlife from xscreensaver")
        , f(new field) {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;
    std::unique_ptr<struct field> f;

    unsigned int cycle_colors = 2;
    unsigned int colortimer = 0;
    uint32_t cycling_color = 0;

    int density = 32, cycles=0;
    ImVec4 clear_color = ImVec4(1, 0, 0, 1.00f);
    ImVec4 background = ImVec4(0, 0, 0, 1);
    ImVec4 foreground = ImVec4(0, 1, 0, 1);

    //pal_t pal = colormap::palettes.at("inferno");
    int item_current_idx = 0;



    unsigned char *cell_at(unsigned int x, unsigned int y);
    unsigned char *new_cell_at(unsigned int x, unsigned int y);
    void resize_field(int fw, int fh);
    void populate_field(unsigned int p);
    void populate_edges(unsigned int p);
    void draw_field();
    void refield();
    unsigned int is_alive(unsigned int x, unsigned int y);
    unsigned int do_tick();

};
