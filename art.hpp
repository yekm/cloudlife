#include <cstdint>
#include <string>
#include <vector>

class Art {
public:
    Art(std::string _name)
        : name(_name) {}
    virtual bool render_gui() = 0;
    virtual void resize(int _w, int _h) = 0;
    virtual void render(uint32_t *p) = 0;
    virtual void load(std::string json) {};
    virtual std::string save() { return ""; };

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
    uint32_t *data() { return pixels.data(); }
    std::vector<uint32_t> pixels;
    std::string name;
};
