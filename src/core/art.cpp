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
    easel->gui();

    bool resize_pbo = render_gui();

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
    double elapsed = current_time - last_shuffle;
    if (elapsed > shuffle_period) {
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