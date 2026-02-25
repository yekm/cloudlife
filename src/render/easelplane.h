#pragma once
#include "easel.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers


class EaselPlane : public Easel {
public:
    EaselPlane();
    ~EaselPlane();

    //void append(Pixel && p);
    void drawdot(int32_t x, int32_t y, uint32_t c) override;

    virtual void begin() override;
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
    int texture_size_bytes();
    int texture_size_pixels();

    typedef uint32_t pixel_t;
    pixel_t *image_data = NULL;
    std::vector<pixel_t> image_data_vector;

    GLuint image_texture;
    GLuint pboIds[2];
    int pbo_index = 0;
    pixel_t* m_plane;
};
