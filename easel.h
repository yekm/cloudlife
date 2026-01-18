#pragma once
#include <vector>
#include <stdint.h>
#include <type_traits>
#include <algorithm>

#include "settings.hpp"

template <int V, typename T>
static inline void fillV(T &container) {
    using ValueType = typename T::value_type;

    if constexpr (std::is_pointer_v<ValueType>) {
        // Use reinterpret_cast for pointers (0 becomes nullptr)
        std::fill(container.begin(), container.end(), reinterpret_cast<ValueType>(V));
    } else {
        // Use static_cast for standard numeric types
        std::fill(container.begin(), container.end(), static_cast<ValueType>(V));
    }
}

template <typename T>
static inline void
fill0(T &container) {
    fillV<0, T>(container);
}

template <int V, typename T>
static inline void
fillV(T container, size_t N) {
    std::fill_n(container, N,
        static_cast<typename std::remove_pointer_t<T>>(V));
}

template <typename T>
static inline void
fill0(T &container, size_t N) {
    fillV<0, T>(container, N);
}



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

    PaletteSetting pal;

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

