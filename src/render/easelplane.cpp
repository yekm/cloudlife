#include "easelplane.h"

#include <stdio.h>

#include "easel.h"
#include "imgui.h"

// GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, GL_MIRRORED_REPEAT, GL_REPEAT, GL_MIRROR_CLAMP_TO_EDGE

int EaselPlane::texture_size_bytes() {
    return w * h * sizeof(pixel_t);
}

int EaselPlane::texture_size_pixels() {
    return w * h;
}

void EaselPlane::make_pbos() {
    image_data_vector.resize(texture_size_bytes());
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
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size_bytes(),
            0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size_bytes(),
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
    clear();
    destroy_pbos();
}

void EaselPlane::reset() {
    //m_plane.resize(w*h);
    destroy_pbos();
    make_pbos();
}

void EaselPlane::begin() {
    // PBO double-buffering: swap indices so we upload from one buffer
    // while mapping the other for CPU writing
    int read_idx = pbo_index;
    pbo_index = (pbo_index + 1) % 2;
    int write_idx = pbo_index;

    // Upload texture from the read buffer (filled in previous frame)
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[read_idx]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
            w, h, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    // Unmap current buffer if mapped
    if (m_plane) {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        m_plane = nullptr;
    }

    // Map the write buffer for CPU pixel data generation
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[write_idx]);
    
    // Orphan buffer to avoid synchronization stalls
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size_bytes(),
            nullptr, GL_STREAM_DRAW);

    // Map the buffer using glMapBufferRange
    m_plane = (uint32_t*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, texture_size_bytes(), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

    if (!m_plane) {
        fprintf(stderr, "ERROR: Failed to map PBO buffer (err = 0x%x)\n", glGetError());
        // Fallback or just ignore, drawing will crash if m_plane is null
    }
}


void EaselPlane::render() {
    if (m_plane) {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        m_plane = nullptr;
    }
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

    if (!m_plane) {
        return;
    }

    m_plane[ y*w + x ] = c;
    ++pixels_drawn;
}

void EaselPlane::gui() {
    pal.RenderGui();

    ImGui::Text("pixels drawn %d, discarded %d",
        pixels_drawn, pixels_discarded);
    ImGui::Text("texture %d x %d", w, h);
    ImGui::Text("window %d x %d", ww, wh);
}

void EaselPlane::clear() {
    begin();
    if (m_plane == nullptr)
        return;
    fill0(m_plane, texture_size_pixels());
    begin();
    if (m_plane == nullptr)
        return;
    fill0(m_plane, texture_size_pixels());
    if (m_plane) {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        m_plane = nullptr;
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void EaselPlane::clear1() {
    if (m_plane == nullptr)
        return;
    fill0(m_plane, texture_size_pixels());
}
