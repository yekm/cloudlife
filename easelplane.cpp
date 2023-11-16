#include "easelplane.h"

#include <stdio.h>

#include "imgui.h"

// GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT, GL_REPEAT, GL_MIRROR_CLAMP_TO_EDGE

int EaselPlane::texture_size() {
    return w * h * sizeof(pixel_t);
}

void EaselPlane::make_pbos() {
    image_data_vector.resize(texture_size());
    image_data = image_data_vector.data();

    // AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
    // http://www.songho.ca/opengl/gl_pbo.html
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D,
            0, GL_RGBA, w, h,
            0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(2, pboIds);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size(),
            0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size(),
            0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void EaselPlane::destroy_pbos() {
    if (image_data_vector.size() == 0)
        return;

    glDeleteTextures(1, &image_texture);
    glDeleteBuffers(2, pboIds);
}


EaselPlane::EaselPlane()
{
    // make_pbos() done in reset() after resize()
}

EaselPlane::~EaselPlane() {
    destroy_pbos();
}

void EaselPlane::reset() {
    m_plane.resize(w*h);
    destroy_pbos();
    make_pbos();
}

void EaselPlane::render() {
    int nexti = pbo_index;
    pbo_index = pbo_index ? 0 : 1;
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[pbo_index]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            w, h, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    //glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);


    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nexti]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size(),
            0, GL_STREAM_DRAW);
    uint32_t* ptr = (uint32_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
    assert(ptr);
    //art->draw(ptr);
    std::copy(m_plane.begin(), m_plane.end(), ptr);

    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)image_texture,
        ImVec2(0, 0), ImVec2(ww, wh),
        ImVec2(0, 0), ImVec2((float)ww/w, (float)wh/h));
}


void EaselPlane::drawdot(int32_t x, int32_t y, uint32_t c) {
    if (x >= w || y >= h) {
        ++pixels_discarded;
        return;
    }

    m_plane[ y*w + x ] = c;
    ++pixels_drawn;
}

void EaselPlane::gui() {
    ImGui::Text("pixels drawn %d, discarded %d",
        pixels_drawn, pixels_discarded);
}
