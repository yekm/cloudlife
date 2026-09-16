#include "art.hpp"
#include "imgui.h"

#include "settings.hpp"
#include <string>

class AcidWarp : public Art {
public:
    std::string about() const override
    {
        return "AcidWarp is Noah Spurrier's mathematical eye-candy program, credited here to 1992 and 1993. "
               "Steven Wills ported the DOS program to Linux and SVGAlib. The archived README describes the "
               "DOS 4.2 release and the Linux 1.0 port, including the use of integer approximations on "
               "machines without floating-point hardware. Pavel Vasilyev's Dear ImGui adaptation is dated "
               "June 2023 in this implementation.\n\n"
               "Each image is a scalar mathematical field sampled over a rectangular pixel grid. "
               "Coordinates are measured relative to the image center; lookup-table approximations provide "
               "distance, angle, sine, and cosine. More than forty selectable functions combine these "
               "ingredients into concentric waves, angular rays, spirals, multi-center interference, stars, "
               "and bitwise patterns. Random offsets give some functions different interference centers "
               "when an image is regenerated.\n\n"
               "The function produces a palette index rather than a final RGB color. For example, the first "
               "pattern adds an angular field to a radial sine wave and horizontal and vertical cosine "
               "waves. A ring field varies with distance; an angular field assigns the same value along a "
               "ray. Combining them bends color bands into apparently moving shapes. The stored index image "
               "remains fixed during palette animation, while its lookup colors change, so much of the "
               "visible movement requires no movement of the underlying pixels.\n\n"
               "The animation proceeds through image generation, fade-in, palette rotation, and fade-out. "
               "Image func chooses the field; pal num chooses the program's own palette family. Frames each "
               "state controls the duration of each animation phase in rendered frames, so actual elapsed "
               "time depends on playback speed. Fade_dir chooses fading toward black or white. Selecting "
               "another image or palette restarts the sequence. These controls let you explore geometry "
               "separately from the color cycle and compare how one mathematical field looks under several "
               "palettes.\n\n"
               "Further reading:\n"
               "https://www.noah.org/acidwarp/\n"
               "https://en.wikipedia.org/wiki/Color_cycling\n"
               "https://en.wikipedia.org/wiki/Procedural_generation";
    }

    AcidWarp()
        : Art("AcidWarp") {}
private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;

    enum { IMAGE, FADE_IN, ROTATE, FADE_OUT } acid_state = IMAGE;

    int image_func = 0, pal_num = 0;
    int frame = 0, frame_max = 60*10;
    bool fade_dir = true;

};

