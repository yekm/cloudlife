#pragma once
#include "easel.h"

class EaselPlane : public Easel {
public:
    EaselPlane();
    ~EaselPlane();

    //void append(Pixel && p);
    void drawdot(int32_t x, int32_t y, uint32_t c) override;

    virtual void render() override;
    //virtual void clear() override;
    //virtual void gui() override;
    virtual void reset() override;

private:
    unsigned pixels_drawn = 0;
    unsigned pixels_discarded = 0;

    void make_pbos();
    void destroy_pbos();
    int texture_size();

    typedef uint32_t pixel_t;
    pixel_t *image_data = NULL;
    std::vector<pixel_t> image_data_vector;

    GLuint image_texture;
    GLuint pboIds[2];
    int pbo_index = 0;
    std::vector<uint32_t> m_plane;
};

