#pragma once

#include "art.hpp"

class Test3D : public Art {
public:
    Test3D() : Art("Test 3D") {
        useVertex3D();
    }
    
private:
    bool render_gui() override;
    bool render(uint32_t *p) override;

    int points_per_frame = 1000;
};
