#include "art.hpp"
#include "imgui.h"
#include <vector>

class OnePixel : public Art {
public:
    OnePixel()
        : Art("One pixel benchmark") {}
private:
    //virtual bool render_gui() override {};
    //virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override {
        drawdot(p, w/2, h/2, 0x00ffffff);
        return true;
    }
};
