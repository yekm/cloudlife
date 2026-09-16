#include <assert.h>

#include "art.hpp"

#include "imgui.h"
#include "imgui_elements.h"

#include "easelplane.h"
#include "easelvertex.h"
#include "easelcompute.h"
#include "easelvertex3d.h"
#include "screenshot.hpp"

Art::Art(std::string _name)
    : m_name(_name)
{
}


const char* Art::name() {
    return m_name.c_str();
}

void Art::resized(int _w, int _h) {
    if (!easel)
        usePlane();
    frame_number = 0;
    easel->set_window_size(_w, _h);
    easel->set_texture_size(_w, _h);
    resize(_w, _h);
}

bool Art::gui() {
    if (ImGui::Button("Shuffle"))
        shuffle();
    ImGui::SameLine();
    if (ImGui::Button("About"))
        m_show_about = true;
    easel->gui();

    bool resize_pbo = render_gui();

    if (m_show_about) {
        ImGui::SetNextWindowSize(ImVec2(720, 600), ImGuiCond_FirstUseEver);
        const std::string title = "About " + m_name + "###ArtAbout";
        if (ImGui::Begin(title.c_str(), &m_show_about, ImGuiWindowFlags_HorizontalScrollbar)) {
            const std::string description = about();
            ImGui::PushTextWrapPos(0.0f);
            size_t start = 0;
            while (start < description.size()) {
                const size_t end = description.find('\n', start);
                const std::string line = description.substr(start, end - start);
                if (line.compare(0, 8, "https://") == 0 || line.compare(0, 7, "http://") == 0) {
                    ImGui::PushID(static_cast<int>(start));
                    ImGui::TextLinkOpenURL(line.c_str());
                    ImGui::PopID();
                } else if (line.empty()) {
                    ImGui::Spacing();
                } else {
                    ImGui::TextUnformatted(line.c_str());
                }
                if (end == std::string::npos)
                    break;
                start = end + 1;
            }
            ImGui::PopTextWrapPos();
        }
        ImGui::End();
    }

    return resize_pbo;
}

void Art::draw() {
    easel->begin();
    render(0);
    easel->render();

    ++frame_number;
}

void Art::drawdot(float x, float y) {
    evertex()->dab(x,y);
}


void Art::clear() {
    easel->clear();
}

void Art::default_resize(int _w, int _h) {
    clear();
}

void Art::usePlane() {
    easel = std::make_unique<EaselPlane>();
    ep = dynamic_cast<EaselPlane*>(easel.get());
    ev = nullptr;
    ec = nullptr;
    ev3d = nullptr;
}
void Art::useVertex() {
    easel = std::make_unique<EaselVertex>();
    ep = nullptr;
    ev = dynamic_cast<EaselVertex*>(easel.get());
    ec = nullptr;
    ev3d = nullptr;
}
void Art::useCompute() {
#ifndef __APPLE__
    easel = std::make_unique<EaselCompute>();
    ep = nullptr;
    ev = nullptr;
    ec = dynamic_cast<EaselCompute*>(easel.get());
    ev3d = nullptr;
#else
    ec = nullptr;
#endif
}
void Art::useVertex3D() {
    easel = std::make_unique<EaselVertex3D>();
    ep = nullptr;
    ev = nullptr;
    ec = nullptr;
    ev3d = dynamic_cast<EaselVertex3D*>(easel.get());
}

EaselPlane* Art::eplane() const {
    assert(ep);
    return ep;
    //return dynamic_cast<EaselPlane*>(easel.get());
}
EaselVertex* Art::evertex() const {
    assert(ev);
    return ev;
    //return dynamic_cast<EaselVertex*>(easel.get());
}
EaselCompute* Art::ecompute() const {
    assert(ec);
    return ec;
    //return dynamic_cast<EaselCompute*>(easel.get());
}
EaselVertex3D* Art::evertex3d() const {
    assert(ev3d);
    return ev3d;
}

void Art::check_shuffle(double current_time) {
    if (shuffle_period <= 0)
        return;

    // Start each art's interval on its first update, even when selected long
    // after application startup. Restart the interval if the clock is reset.
    if (last_shuffle < 0 || current_time < last_shuffle) {
        last_shuffle = current_time;
        return;
    }

    if (current_time - last_shuffle >= shuffle_period) {
        shuffle();
        last_shuffle = current_time;
    }
}

void Art::save_frame() {
    std::string filename = generate_screenshot_filename(m_name, frame_number);
    if (save_framebuffer_to_png(filename, easel->ww, easel->wh)) {
        printf("Saved screenshot: %s\n", filename.c_str());
    } else {
        printf("Failed to save screenshot: %s\n", filename.c_str());
    }
}
