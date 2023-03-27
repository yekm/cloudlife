
#include "settings.hpp"

#include "imgui.h"

#include "imgui_elements.h"


bool PaletteSetting::RenderGui() {
    bool ret = false;
    auto pb = colormap::palettes.begin();
    std::advance(pb, item_current_idx);

    int size = colormap::palettes.size();
    const char* combo_preview_value = pb->first.c_str();
    if (ImGui::BeginCombo("Palette", combo_preview_value, 0))
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
