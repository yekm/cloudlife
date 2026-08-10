#include "test3d.hpp"

#include "imgui.h"
#include "imgui_elements.h"
#include "easelvertex3d.h"
#include "random.h"
#include <cmath>

bool Test3D::render_gui() {
    bool changed = false;
    if (ImGui::CollapsingHeader("Test3D Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        changed = ScrollableSliderInt("Points per frame", &points_per_frame, 10, 10000, "%d", 1);
    }
    return changed;
}

bool Test3D::render(uint32_t *p) {
    EaselVertex3D* e3d = evertex3d();

    for (int i = 0; i < points_per_frame; ++i) {
        float x = balance_rand(4.0f);
        float y = balance_rand(4.0f);
        float z = balance_rand(4.0f);
        
        float dist = std::sqrt(x*x + y*y + z*z);
        float color_idx = dist / 4.0f;
        
        e3d->drawdot(x, y, z, color_idx);
    }
    
    return true;
}
