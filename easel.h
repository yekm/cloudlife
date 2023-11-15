#pragma once
#include <vector>
//#include <deque>
#include <stdint.h>

#include "imgui.h"

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GLFW/glfw3.h> // Will drag system OpenGL headers


#if 0
// attempt to make drawdot call not virtual
enum class Etype { PLANE, PIXEL, VERTEX };
template<Etype E = Etype::VERTEX>
class Easel {
public:
};
#endif

class Easel {
public:
    Easel() = default;
    virtual ~Easel() = default;

    // every drawdot call is vertual now :(
    virtual void drawdot(int32_t x, int32_t y, uint32_t c) = 0;

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
        w = _w; h = _h; // TODO: or do set_texture_size() in Art::default_resize?
        reset();
    }
    void set_texture_size(int _w, int _h) {
        w = _w; h = _h;
        reset();
    }
    int w, h;
    int ww, wh;
};


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

