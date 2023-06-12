#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>


#include "pixelbuffer.h"
#include "imgui.h"
#include "imgui_elements.h"


template <typename T>
static inline void
fill0(T &container) {
    std::fill(container.begin(), container.end(), 0);
}

class Art {
public:
    Art(std::string _name)
        : m_name(_name) {}
    const char * name() {return m_name.c_str();}
    void resized(int _w, int _h) { // nb: old buffers -> forbidden to drawdot()
        tex_w = _w, tex_h = _h;
        resize(_w, _h);
    }
    bool gui() {
        if (pb)
            ScrollableSliderUInt("Max pixels", &pixel_buffer_maximum, 1, pixel_buffer_maximum_max, "%d", pixel_buffer_maximum/16);
        bool resize_pbo = render_gui();

        ImGui::Text("pixels drawn %d, discarded %d",
            pixels_drawn, pixels_discarded);

        if (pb)
            ImGui::Text("pixel_buffer_size %ld",
                pb->buffer.size());

        return resize_pbo;
    }
    void draw(uint32_t *p) {
        bool direct = render(p);
        if (pb) {
            pb->erase_old(pixel_buffer_maximum);
            render_pixel_buffer(p);
        } else if (!direct) {
            std::copy(pixels.begin(), pixels.end(), p);
        }
    }
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

    void drawdot(uint32_t *screen, uint32_t x, uint32_t y, uint32_t c) {
        if (x >= tex_w || y >= tex_h) {
            ++pixels_discarded;
            return;
        }
        screen[ y*tex_w + x ] = c;
        ++pixels_drawn;
    }

    virtual void reinit() { resize(w, h); }

    virtual ~Art() = default;

    unsigned frame_number = 0, clear_every = 0;
    unsigned pixel_buffer_maximum = 1024*10, pixel_buffer_maximum_max = 1024*1024;

    void clear() {
        fill0(pixels);
        pixels_drawn = 0;
        pixels_discarded = 0;
    }

    int tex_w, tex_h;
private:
    virtual void resize(int _w, int _h) {default_resize(_w, _h);};
    virtual bool render_gui() = 0;
    virtual bool render(uint32_t *p) = 0;

    void render_pixel_buffer(uint32_t *screen) {
        std::fill_n(screen, w*h, 0);
#if 1
        auto b = pb->buffer.begin();
        auto pbm = pb->buffer.size() > pixel_buffer_maximum ? pixel_buffer_maximum : pb->buffer.size();
        auto end = pb->buffer.begin() + pbm;
        for (; b != end; ++b)
            drawdot(screen, b->x, b->y, b->color);
#else
        for (auto &p : pb->buffer)
            drawdot(screen, p.x, p.y, p.color);
#endif
    }


protected:
    unsigned pixels_drawn = 0;
    unsigned pixels_discarded = 0;
    void default_resize(int _w, int _h) {
        w = _w;
        h = _h;
        //data = (uint8_t *)xrealloc(data, w*h*sizeof(uint32_t));
        pixels.resize(w*h);
        clear();
        /* TODO: fill square in drawdot() if pscale > 1
        pscale = 1;
        if (xgwa.width > 2560 || xgwa.height > 2560)
            pscale = 3; // Retina displays
        */

    }
    int w, h;
    //uint8_t *data() { return reinterpret_cast<uint8_t*>(pixels.data()); }
    uint32_t *data() { return pixels.data(); }
    std::vector<uint32_t> pixels;
    std::string m_name;
    std::unique_ptr<PixelBuffer> pb;

};

