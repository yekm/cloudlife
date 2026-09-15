#include "easelplane.h"

#include "imgui.h"

#include <cstdio>
#include <cstring>

size_t EaselPlane::texture_size_bytes() const
{
    return texture_size_pixels() * sizeof(pixel_t);
}

size_t EaselPlane::texture_size_pixels() const
{
    return static_cast<size_t>(w) * static_cast<size_t>(h);
}

void EaselPlane::make_pbos()
{
    if (w <= 0 || h <= 0)
        return;

    image_data_vector.assign(texture_size_pixels(), 0);

    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // This is a CPU pointer, so no unpack buffer may be bound.
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h,
            0, GL_RGBA, GL_UNSIGNED_BYTE, image_data_vector.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    // Storage is allocated and completely initialized before each upload.
    glGenBuffers(2, pboIds);
}

void EaselPlane::destroy_pbos()
{
    if (image_texture != 0)
        glDeleteTextures(1, &image_texture);
    if (pboIds[0] != 0 || pboIds[1] != 0)
        glDeleteBuffers(2, pboIds);
    image_texture = 0;
    pboIds[0] = pboIds[1] = 0;
    pbo_index = 0;
    image_data_vector.clear();
}

EaselPlane::EaselPlane()
{
    // make_pbos() done in reset() after resize()
}

EaselPlane::~EaselPlane()
{
    destroy_pbos();
}

void EaselPlane::reset()
{
    destroy_pbos();
    make_pbos();
}

void EaselPlane::upload_image()
{
    if (image_data_vector.empty())
        return;

    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[pbo_index]);
    // PBOs are staging buffers, never the backing store for incremental drawing.
    // Orphaning lets a previous upload finish while we copy the current CPU image.
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size_bytes(), nullptr, GL_STREAM_DRAW);
    void* mapped = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, texture_size_bytes(),
            GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
    bool copied = false;
    if (mapped) {
        std::memcpy(mapped, image_data_vector.data(), texture_size_bytes());
        copied = glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER) == GL_TRUE;
    }
    if (!copied) {
        fprintf(stderr, "ERROR: Failed to map or unmap PBO; retrying with CPU image (err = 0x%x)\n",
                glGetError());
        // A failed unmap invalidates the PBO contents. Recreate them from the CPU image.
        glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size_bytes(),
                image_data_vector.data(), GL_STREAM_DRAW);
    }

    // Upload only initialized, unmapped storage, including on the very first frame.
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    pbo_index = (pbo_index + 1) % 2;
#ifndef NDEBUG
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
        fprintf(stderr, "ERROR: Failed to upload plane image (err = 0x%x)\n", error);
#endif
}

void EaselPlane::render()
{
    if (image_data_vector.empty())
        return;

    upload_image();
    ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)image_texture,
        ImVec2(0, 0), ImVec2(ww, wh),
        ImVec2(0, 0), ImVec2((float)ww/w, (float)wh/h));
}

void EaselPlane::drawdot(int32_t x, int32_t y, uint32_t c)
{
    if (x < 0 || y < 0 || x >= w || y >= h) {
        ++pixels_discarded;
        return;
    }

    if (image_data_vector.empty())
        return;

    image_data_vector[static_cast<size_t>(y) * static_cast<size_t>(w) + x] = c;
    ++pixels_drawn;
}

void EaselPlane::gui()
{
    pal.RenderGui();

    ImGui::Text("pixels drawn %u, discarded %u",
        pixels_drawn, pixels_discarded);
    ImGui::Text("texture %d x %d", w, h);
    ImGui::Text("window %d x %d", ww, wh);
}

void EaselPlane::clear()
{
    clear1();
    upload_image();
}

void EaselPlane::clear1()
{
    fill0(image_data_vector);
}
