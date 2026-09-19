#pragma once
#include "easel.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers


class EaselPlane : public Easel {
public:
    EaselPlane();
    ~EaselPlane() override;

    //void append(Pixel && p);
    void drawdot(int32_t x, int32_t y, uint32_t c) override;
    // Read the persistent CPU image without GPU readback. Out-of-bounds reads return 0.
    uint32_t read_pixel(int32_t x, int32_t y) const;

    virtual void render() override;
    virtual void clear() override;
    void clear1();
    virtual void gui() override;
    virtual void reset() override;

private:
    unsigned pixels_drawn = 0;
    unsigned pixels_discarded = 0;

    void make_pbos();
    void destroy_pbos();
    void upload_image();
    size_t texture_size_bytes() const;
    size_t texture_size_pixels() const;

    using pixel_t = uint32_t;
    // The CPU image persists across frames, including frames that only update a few pixels.
    std::vector<pixel_t> image_data_vector;

    GLuint image_texture = 0;
    GLuint pboIds[2] = {0, 0};
    int pbo_index = 0;
};
