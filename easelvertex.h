#pragma once
#include "easel.h"

#include "imgui.h"

#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GLFW/glfw3.h> // Will drag system OpenGL headers


class EaselVertex : public Easel {
public:
    EaselVertex();
    ~EaselVertex();

    void dab(float x, float y);
    void drawdot(int32_t x, int32_t y, uint32_t c) override;
    void drawdot(float x, float y); // non virtual

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
    ImVec4 vertex_color = ImVec4(1, 0.5, 0.2, 0.05f);

    std::string fragmentShaderSource;
    bool use_colormap = true, update_opacity = true;
    float cmap_opacity = 0.4;
};
