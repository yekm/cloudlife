#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>


#include "easel.h"
//#include "easelplane.h"
//#include "easelvertex.h"


class Art {
public:
    Art(std::string _name);
    const char * name();

    /* called when main window is resized and if reinit needed.
     * after this call PBOs will be recreated */
    void resized(int _w, int _h);

    /* returns true if there is a need to recreate PBOs */
    bool gui();

    /* draws a picture into tex_w x tex_h uint32_t RGBA memory buffer */
    void draw();

    /* TODO: load store of current gui configns */
    virtual void load(std::string json) {};
    virtual std::string save() { return ""; };

    void drawdot(uint32_t *p, int32_t x, int32_t y, double o, uint32_t c) {
        drawdot(x, y, o, c);
    }
    void drawdot(int32_t x, int32_t y, double o, uint32_t c) {
        drawdot(x, y, c | ((unsigned)(0xff*o)<<24));
    }
    void drawdot(uint32_t *p, int32_t x, int32_t y, uint32_t c) {
        drawdot(x,y,c);
    }

    void drawdot(int32_t x, int32_t y, uint32_t c) {
        easel->drawdot(x,y,c);
    }

    void drawdot(float x, float y);

    virtual ~Art() = default;


    unsigned frame_number = 0, clear_every = 0, max_kframes = 0;

    void clear();

private:
    virtual void resize(int _w, int _h) { default_resize(_w, _h); };
    virtual bool render_gui() {return false;}
    virtual bool render(uint32_t *p) = 0;

    void render_pixel_buffer(uint32_t *screen);

protected:
    void default_resize(int _w, int _h);

    std::unique_ptr<Easel> easel;
    EaselPlane * ep = nullptr;
    EaselVertex * ev = nullptr;
    void usePlane();
    void useVertex();
    EaselPlane* eplane() const;
    EaselVertex* evertex() const;

    std::string m_name;
};

