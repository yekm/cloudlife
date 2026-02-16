#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"

#include <vector>
#include <memory>


struct field {
    unsigned int height = 0;
    unsigned int width = 0;
    unsigned int max_age = 64;
    unsigned int cell_size = 3;
    unsigned int ticks_per_frame = 1;
    std::vector<uint8_t> cells, new_cells;
};

class Cloudlife : public Art {
public:
    Cloudlife()
        : Art("Cloudlife from xscreensaver")
        , f(new field) {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;
    std::unique_ptr<struct field> f;

    unsigned ncolors=512;
    unsigned int colorindex = ncolors;  /* which color in the colormap are we on */
    unsigned int colortimer = 1;  /* when this reaches 0, cycle to next color */

    int density = 32, cycles=0;
    ImVec4 clear_color = ImVec4(1, 0, 0, 1.00f);
    ImVec4 background = ImVec4(0, 0, 0, 1);
    ImVec4 foreground = ImVec4(0, 1, 0, 1);

    //pal_t pal = colormap::palettes.at("inferno");
    int item_current_idx = 0;



    uint32_t get_color_age(int age);
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

