
#include "settings.hpp"

#include "imgui.h"

#include "imgui_elements.h"


bool PaletteSetting::RenderGui() {
    bool ret = false;
    auto pb = colormap::palettes.begin();
    std::advance(pb, item_current_idx);

    int size = colormap::palettes.size();
    const char* combo_preview_value = pb->first.c_str();
    if (ImGui::BeginCombo(name.c_str(), combo_preview_value, 0))
    {
        auto pb = colormap::palettes.begin();
        for (int n = 0; n < size; n++)
        {
            const bool is_selected = (item_current_idx == n);
            const char * name = pb->first.c_str();
            if (ImGui::Selectable(name, is_selected))
            {
                item_current_idx = n;
                value = pb->second;
                ret = true;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
            std::advance(pb, 1);
        }
        ImGui::EndCombo();
    }
    return ret;
}

void PaletteSetting::rescale(uint32_t ncolours) {
    value = value.rescale(0., ncolours);
    color_max = ncolours;
}

uint32_t PaletteSetting::get_color(uint32_t color_n) {
    if (color_n > color_max)
        printf("%d>%d ", color_n, (int)color_max);
    auto c = value(color_n);
    return 0xff000000 |
            c.getRed().getValue() << 0 |
            c.getGreen().getValue() << 8 |
            c.getBlue().getValue() << 16;
}




bool VectorCombo::RenderGui() {
    bool ret = false;
    auto pb = items.begin();
    std::advance(pb, item_current_idx);

    int size = items.size();
    const char* combo_preview_value = pb->c_str();
    if (ImGui::BeginCombo(name.c_str(), combo_preview_value, 0))
    {
        auto pb = items.begin();
        for (int n = 0; n < size; n++)
        {
            const bool is_selected = (item_current_idx == n);
            const char * name = pb->c_str();
            if (ImGui::Selectable(name, is_selected))
            {
                item_current_idx = n;
                value = *pb;
                ret = true;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
            std::advance(pb, 1);
        }
        ImGui::EndCombo();
    }
    return ret;
}

void VectorCombo::set_index(int i) {
    item_current_idx = i;
}

int VectorCombo::get_index() {
    return item_current_idx;
}

std::string VectorCombo::get_value() {
    return items.at(get_index());
}
