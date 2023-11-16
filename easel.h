#pragma once
#include <vector>
#include <stdint.h>


class Easel {
public:
    Easel() = default;
    virtual ~Easel() = default;

    // every drawdot call is vertual now :(
    virtual void drawdot(int32_t x, int32_t y, uint32_t c) = 0;

    virtual void begin() {};
    virtual void render() = 0;
    virtual void clear() {};
    virtual void gui() {};
    virtual void reset() {};


    // TODO: ugly pieces from vertex easel, move to a better place
    unsigned frame_vertex_target_k = 128;
    unsigned frame_vertex_target() const {
        return frame_vertex_target_k * 1024;
    }

    unsigned vertex_buffer_maximum_k = 1024*16;
    unsigned vertex_buffer_maximum() const {
        return vertex_buffer_maximum_k * 1024;
    }


    // TODO: ugly pieces from plane easel, move to a better place
    void set_window_size(int _w, int _h) {
        ww = _w; wh = _h;
        //w = _w; h = _h; // TODO: or do set_texture_size() in Art::default_resize?
        //reset();
    }
    void set_texture_size(int _w, int _h) {
        w = _w; h = _h;
        reset();
    }
    int w, h;
    int ww, wh;
};

class EaselPlane;
class EaselVertex;


// deprecated
/*
class EaselPixel : public Easel {
public:

    EaselPixel(unsigned max_vertices);
    ~EaselPixel();

    void append(Pixel && p);

    virtual void render() override;
    virtual void clear() override;
    virtual void gui() override;

private:
    unsigned pixels_drawn = 0;
    unsigned pixels_discarded = 0;
    std::deque<Pixel> buffer;
};
*/

