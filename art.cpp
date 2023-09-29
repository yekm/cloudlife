#include "art.hpp"

#include "imgui.h"
#include "imgui_elements.h"

const char* Art::name() {
    return m_name.c_str();
}

void Art::resized(int _w, int _h) {
    if (use_vertex_buffer)
        vb = std::make_unique<VertexBuffer>(vertex_buffer_maximum());
    else
        if (use_pixel_buffer)
            pb = std::make_unique<PixelBuffer>();

    tex_w = _w, tex_h = _h;
    frame_number = 0;
    resize(_w, _h);
}

bool Art::gui() {
    if (ImGui::Checkbox("Force use vertex buffer", &use_vertex_buffer)) {
        if (use_vertex_buffer)
            vb = std::make_unique<VertexBuffer>(vertex_buffer_maximum());
        else
            vb.reset();
    }

    if (!use_vertex_buffer)
        if (ImGui::Checkbox("Force use pixel buffer", &use_pixel_buffer)) {
            if (use_pixel_buffer)
                pb = std::make_unique<PixelBuffer>();
            else
                pb.reset();
        }

    if (pb)
        ScrollableSliderUInt("Max pixels", &pixel_buffer_maximum, 1, pixel_buffer_maximum_max, "%d", pixel_buffer_maximum/16);
    if (vb) {
        if (ScrollableSliderUInt("Max vertex", &vertex_buffer_maximum_k, 1, 1024*32, "%d", 32)) {
            vb = std::make_unique<VertexBuffer>(vertex_buffer_maximum());
        }
        ScrollableSliderUInt("Frame vertex target", &frame_vertex_target_k, 1, 1024, "%d", 8);

    }

    bool resize_pbo = render_gui();

    ImGui::Text("pixels drawn %d, discarded %d",
        pixels_drawn, pixels_discarded);

    if (pb)
        ImGui::Text("pixel_buffer_size %ld",
            pb->buffer.size());
    if (vb)
        ImGui::Text("total vertecies %d",
            vb->total_vertices);

    return resize_pbo;
}

void Art::draw(uint32_t* p) {
    bool direct = render(p);
    if (vb)
        vb->draw();
    else
        if (pb) {
            pb->erase_old(pixel_buffer_maximum);
            render_pixel_buffer(p);
        } else if (!direct) {
            std::copy(m_pixels.begin(), m_pixels.end(), p);
        }
}

void Art::drawdot(uint32_t* screen, uint32_t x, uint32_t y, uint32_t c) {
    if (x >= tex_w || y >= tex_h) {
        ++pixels_discarded;
        return;
    }

    if (vb)
        vb->adot((float)x/tex_w-.5, (float)y/tex_h-.5);
    else
        if (pb)
            pb->append({x, y, c});
        else
            really_drawdot(screen, x, y, c);

    ++pixels_drawn;
}

void Art::clear() {
    fill0(m_pixels);
    pixels_drawn = 0;
    pixels_discarded = 0;
    if (vb)
        vb->clear();
}

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

void Art::default_resize(int _w, int _h) {
    w = _w;
    h = _h;
    //data = (uint8_t *)xrealloc(data, w*h*sizeof(uint32_t));
    m_pixels.resize(w*h);
    clear();
    /* TODO: fill square in drawdot() if pscale > 1
    pscale = 1;
    if (xgwa.width > 2560 || xgwa.height > 2560)
        pscale = 3; // Retina displays
    */

}

