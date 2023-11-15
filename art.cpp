#include "art.hpp"

#include "imgui.h"
#include "imgui_elements.h"

const char* Art::name() {
    return m_name.c_str();
}

void Art::resized(int _w, int _h) {
    tex_w = _w, tex_h = _h;
    frame_number = 0;
    easel->set_window_size(_w, _h);
    resize(_w, _h);
}

bool Art::gui() {
    easel->gui();

    bool resize_pbo = render_gui();

    //ImGui::Text("pixels drawn %d, discarded %d",
    //    pixels_drawn, pixels_discarded);

    return resize_pbo;
}

void Art::draw(uint32_t* p) {
    bool direct = render(p);
    easel->render();
}

void Art::clear() {
    easel->clear();
}
/*
void Art::render_pixel_buffer(uint32_t* screen) {
    std::fill_n(screen, w*h, 0);
#if 1
    auto pbm = std::min<uint64_t>(pb->buffer.size(), pixel_buffer_maximum);// ? pixel_buffer_maximum : pb->buffer.size();
    std::for_each_n(pb->buffer.begin(), pbm, [&](PixelBuffer::Pixel & p) {
        really_drawdot(screen, p.x, p.y, p.color);
    });
#else
    for (auto &p : pb->buffer)
        drawdot(screen, p.x, p.y, p.color);
#endif
}
*/
void Art::default_resize(int _w, int _h) {
    w = _w;
    h = _h;
    //data = (uint8_t *)xrealloc(data, w*h*sizeof(uint32_t));
    //m_pixels.resize(w*h);
    clear();
    /* TODO: fill square in drawdot() if pscale > 1
    pscale = 1;
    if (xgwa.width > 2560 || xgwa.height > 2560)
        pscale = 3; // Retina displays
    */

}
