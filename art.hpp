#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>


#include "pixelbuffer.h"


template <int N, typename T>
static inline void
fillN(T &container) {
    std::fill(container.begin(), container.end(),
        static_cast<typename T::value_type>(N));
}

template <typename T>
static inline void
fill0(T &container) {
    fillN<0, T>(container);
}

class Art {
public:
    Art(std::string _name)
        : m_name(_name) {}
    const char * name();

    /* called when main window is resized and if reinit needed.
     * after this call PBOs will be recreated */
    void resized(int _w, int _h);

    /* returns true if there is a need to recreate PBOs */
    bool gui();

    /* draws a picture into tex_w x tex_h uint32_t RGBA memory buffer */
    void draw(uint32_t *p);

    /* TODO: load store of current gui configns */
    virtual void load(std::string json) {};
    virtual std::string save() { return ""; };

    void drawdot(uint32_t x, uint32_t y, double o, uint32_t c) {
        drawdot(x, y, c | ((unsigned)(0xff*o)<<24));
    }
    void drawdot(uint32_t x, uint32_t y, uint32_t c) {
        drawdot(data(), x, y, c);
    }
    void drawdot(uint32_t *screen, uint32_t x, uint32_t y, double o, uint32_t c) {
        drawdot(screen, x, y, c | ((unsigned)(0xff*o)<<24));
    }

    /* don't really draws a dot if pixel buffer is used */
    void drawdot(uint32_t *screen, uint32_t x, uint32_t y, uint32_t c);

    void really_drawdot(uint32_t *screen, uint32_t x, uint32_t y, uint32_t c) {
        //printf("%x ", c);
        screen[ y*tex_w + x ] = c;
    }

    /* deprecated? */
    virtual void reinit() { resize(w, h); }

    virtual ~Art() = default;

    unsigned frame_number = 0, clear_every = 0, max_kframes = 0;
    unsigned pixel_buffer_maximum = 1024*10, pixel_buffer_maximum_max = 1024*1024;

    /* clears m_pixels */
    void clear();

    /* in order to use custom texture size this variables must be overwritten
     * on each resize(). make_pbo() in main() uses this variables */
    int tex_w, tex_h;
private:
    virtual void resize(int _w, int _h) { default_resize(_w, _h); };
    virtual bool render_gui() {return false;}
    virtual bool render(uint32_t *p) = 0;

    void render_pixel_buffer(uint32_t *screen);
    std::unique_ptr<PixelBuffer> pb;

    uint32_t *data() { return m_pixels.data(); }

protected:
    bool use_pixel_buffer = false;
    unsigned pixels_drawn = 0;
    unsigned pixels_discarded = 0;
    void default_resize(int _w, int _h);
    int w, h;
    std::vector<uint32_t> m_pixels;
    std::string m_name;
};

