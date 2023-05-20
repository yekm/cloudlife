#pragma once

#include <cstdint>
#include <string>
#include <vector>


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
    virtual bool render_gui() = 0;
    virtual void resize(int _w, int _h) {};
    virtual void render(uint32_t *p) = 0;
    virtual void load(std::string json) {};
    virtual std::string save() { return ""; };

    virtual bool override_texture_size(int &w, int &h) { return false; };

    void drawdot(int x, int y, double o, uint32_t c) {
        if (x+1 > w || y+1 > h) return;
        uint32_t *p = data();
        p[ y*w + x ] = c | ((unsigned)(0xff*o)<<24);
    }

    void drawdot(int x, int y, uint32_t c) {
        if (x+1 > w || y+1 > h) return;
        uint32_t *p = data();
        p[ y*w + x ] = c;
    }

    virtual void reinit() { resize(w, h); }

    virtual ~Art() = default;

protected:
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
    void clear() {
        fill0(pixels);
        //std::fill(pixels.begin(), pixels.end(), 0);
    }
    int w, h;
    //uint8_t *data() { return reinterpret_cast<uint8_t*>(pixels.data()); }
    uint32_t *data() { return pixels.data(); }
    std::vector<uint32_t> pixels;
    std::string m_name;
};

