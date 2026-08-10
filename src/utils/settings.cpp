
#include "settings.hpp"

#include "colormap/colormap.h"
#include "imgui.h"
#include "imgui_internal.h"

#include "imgui_elements.h"
#include <algorithm>
#include <iterator>

#include <iostream>

PaletteSetting::PaletteSetting(std::string pname)
    : Setting("Palette") {
    auto l = colormap::ColormapList::getAll();

    for (auto &m : l) {
        names.push_back(m->getTitle());
        maps[m->getTitle()] = m;
    }

    vc = VectorCombo("Colormap", names);
    auto selected = maps.find(pname);
    if (selected == maps.end() && !names.empty())
        selected = maps.find(names.front());

    if (selected != maps.end()) {
        vc.set_index(selected->first);
        current_cmap = selected->second;
    }
}

bool PaletteSetting::RenderGui() {
    ImGui::Checkbox("invert", &invert);
    ImGui::SameLine(0, 1);

    bool ret = vc.RenderGui();
    if (ret) {
        auto selected = maps.find(vc.get_value());
        if (selected != maps.end())
            current_cmap = selected->second;
    }

    ScrollableSliderUInt("Max colors", &color_max, 1, 1024*32, "%d", 128);

    ImGui::Text("current color / max %d / %d", current_color, color_max);
    // ImGui::Text("current color %x", get_color(current_color)); // slow!

    return ret;
}

void PaletteSetting::rescale(uint32_t ncolours) {
    color_max = std::max<uint32_t>(ncolours, 1);
}

uint32_t PaletteSetting::get_color(uint32_t color_n) {
    //if (color_n > color_max)
    //    std::cerr << "color_n > color_max " << color_n << " > " << color_max << std::endl;
    return get_colorf((float)(color_n%color_max)/color_max);
}

uint32_t PaletteSetting::get_colorf(float color_n) const {

    if (invert)
        color_n = 1.0f - color_n;

    if (!current_cmap)
        return 0;

    auto c = current_cmap->getColor(color_n);

    return ImGui::ColorConvertFloat4ToU32({static_cast<float>(c.r),
                                           static_cast<float>(c.g),
                                           static_cast<float>(c.b),
                                           static_cast<float>(c.a)});
}

float PaletteSetting::get_color_index(uint32_t color_n) const {
    return static_cast<float>(color_n) / static_cast<float>(std::max<uint32_t>(color_max, 1));
}

float PaletteSetting::get_next_color_index() {
    ++current_color;
    if (current_color > color_max)
        current_color = 0;
    return (float)current_color / color_max;
}

uint32_t PaletteSetting::get_next_color() {
    return get_colorf(get_next_color_index());
}

const colormap::Colormap & PaletteSetting::get_cmap() {
    return *current_cmap;
}

////////////////////////////////////////////////////////////////////////////////

bool VectorCombo::RenderGui() {
    if (items.empty())
        return false;

    if (item_current_idx < 0 || item_current_idx >= static_cast<int>(items.size()))
        item_current_idx = 0;

    bool ret = false;
    auto pb = items.begin();
    std::advance(pb, item_current_idx);

    int size = items.size();
    const char* combo_preview_value = pb->c_str();
    const bool combo_open = ImGui::BeginCombo(name.c_str(), combo_preview_value, 0);
    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
    if (!combo_open && ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0.0f)
    {
        item_current_idx += ImGui::GetIO().MouseWheel > 0.0f ? -1 : 1;
        if (item_current_idx < 0)
            item_current_idx = size - 1;
        else if (item_current_idx >= size)
            item_current_idx = 0;
        value = items.at(item_current_idx);
        ret = true;
    }

    if (combo_open)
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

void VectorCombo::set_index(std::string p) {
    auto it = std::find(items.begin(), items.end(), p);
    if (it != items.end())
        set_index(static_cast<int>(std::distance(items.begin(), it)));
}

void VectorCombo::set_index(int i) {
    if (i < 0 || i >= static_cast<int>(items.size()))
        return;

    item_current_idx = i;
    value = items.at(item_current_idx);
}

int VectorCombo::get_index() const {
    return item_current_idx;
}

std::string VectorCombo::get_value() const {
    return value;
}
