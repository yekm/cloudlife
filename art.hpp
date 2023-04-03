#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Art {
public:
    typedef uint32_t pixel_t;
    Art(std::string _name)
        : name(_name) {}
    virtual bool render_gui() = 0;
    virtual void resize(int _w, int _h) {};
    virtual void render(uint32_t *p) = 0;
    virtual void load(std::string json) {};
    virtual std::string save() { return ""; };

    virtual bool override_texture_size(int &w, int &h) { return false; };
    virtual int texture_size() { return pixels.size()*sizeof(pixel_t); };
    int tex_w() { return w; };
    int tex_h() { return h; };

    void drawdot(int x, int y, double o, uint32_t c) {
        if (x+1 > w || y+1 > h) return;
        uint32_t *p = texture();
        p[ y*w + x ] = c | ((unsigned)(0xff*o)<<24);
    }

    void drawdot(int x, int y, uint32_t c) {
        if (x+1 > w || y+1 > h) return;
        uint32_t *p = texture();
        p[ y*w + x ] = c;
    }

    virtual void reinit() { resize(w, h); }
    pixel_t *texture() { return pixels.data(); }

protected:
    void default_resize(int _w, int _h) {
        w = _w;
        h = _h;
        //data = (uint8_t *)xrealloc(data, w*h*sizeof(uint32_t));
        pixels.resize(w*h);
        clear();
    }
    void clear() {
        std::fill(pixels.begin(), pixels.end(), 0);
    }
    int w, h;
    //uint8_t *data() { return reinterpret_cast<uint8_t*>(pixels.data()); }
    std::vector<pixel_t> pixels;
    std::string name;
};
