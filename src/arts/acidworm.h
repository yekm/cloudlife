#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"
#include <string>

typedef struct
{
    int orientation, head;
    //short *xpos, *ypos;
    std::vector<short> xpos, ypos;
} worm_t;

class AcidWorm : public Art {
public:
    std::string about() const override
    {
        return "AcidWorm 0.2 was written by Aaron Tiensivu, adapting Eric P. Scott's worm program and "
               "taking visual inspiration from AcidWarp. Its archived Linux software-map entry is dated 17 "
               "December 1996 and identifies it as SVGALib screensaver eye candy. The original README "
               "preserves Tiensivu's attribution and describes variable worm lengths, counts, placement, "
               "and color schemes. This art carries that program into Cloudlife's pixel renderer.\n\n"
               "Every worm stores a heading and a circular history of pixel coordinates. The heading has "
               "eight possible directions on the square grid. Away from the edges, the next heading is "
               "chosen randomly from the current direction and its two adjacent directions, so paths tend "
               "to continue forward with occasional gentle turns. Boundary-specific choice tables keep the "
               "heads inside the image and guide them around corners. A step changes the head coordinate by "
               "an integer vector whose components are -1, 0, or 1.\n\n"
               "Advancing the circular history replaces the oldest body position with the new head "
               "position. A reference-count grid records how many worm segments occupy each pixel. Removing "
               "a tail clears a pixel only when its count reaches zero, which preserves crossings shared by "
               "several worms. Long worms retain more of their path; many short worms produce a dense, "
               "rapidly changing texture. The worm's number selects its color index modulo 256.\n\n"
               "Color_cycle selects the built-in scheme: static RGB, cycling RGB, a monochrome-style "
               "gradient, a sinusoidal palette, or randomized colors. Cycling rotates one randomly chosen "
               "RGB channel through the palette on each update. In this port the palette is converted to "
               "RGB when a pixel is drawn, so already drawn pixels keep their previous RGB value until "
               "touched again.\n\n"
               "Length sets the number of stored body positions; number sets the worm population. Position "
               "chooses a random start, the center, one of four corners, or a randomly selected placement "
               "mode. Changing these controls rebuilds the worms and their histories. Placement mainly "
               "determines where a new population begins; its later shapes emerge from the local heading "
               "rules and overlaps.\n\n"
               "Further reading:\n"
               "https://en.wikipedia.org/wiki/Random_walk\n"
               "https://en.wikipedia.org/wiki/Color_cycling\n"
               "https://www.noah.org/acidwarp/";
    }

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

