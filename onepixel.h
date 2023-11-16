#include "art.hpp"
#include "imgui.h"
#include <vector>

class OnePixel : public Art {
public:
    OnePixel()
        : Art("One pixel benchmark") {
            useVertex();
            //use_pixel_buffer = true;
            //pixel_buffer_maximum = 1024*512;
        }
private:
    //virtual bool render_gui() override {};
    //virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override {
        //drawdot(p, w/2, h/2, 0x00ffffff);
        drawdot(.5, .5);
        return true;
    }
};
