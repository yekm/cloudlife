#pragma once
#include "easel.h"

class EaselVertex : public Easel {
public:
    EaselVertex();
    ~EaselVertex();

    void dab(float x, float y);
    void drawdot(int32_t x, int32_t y, uint32_t c) override;

    virtual void render() override;
    virtual void clear() override;
    virtual void gui() override;

private:
    void init_shaders();
    void create_vertex_buffer();
    void destroy_vertex_buffer();
    unsigned total_vertices = 0;
    unsigned vao, vbo;
    std::vector<float> m_vertices;
    GLuint shaderProgram;
    ImVec4 vertex_color = ImVec4(1, 0.5, 0.2, 0.1f);

};
