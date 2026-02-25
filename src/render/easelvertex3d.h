#pragma once
#include "easel.h"

#include "imgui.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>

class EaselVertex3D : public Easel {
public:
    EaselVertex3D();
    ~EaselVertex3D();

    void drawdot(int32_t x, int32_t y, uint32_t c) override;
    void drawdot(float x, float y, float z, float c);

    virtual void render() override;
    virtual void clear() override;
    virtual void gui() override;

private:
    void init_shaders();
    void reload_shaders();
    void build_fragment_shader_source();
    void create_vertex_buffer();
    void destroy_vertex_buffer();
    
    unsigned total_vertices = 0;
    unsigned vao, vbo;
    GLuint shaderProgram;

    std::string fragmentShaderSource;
    bool use_colormap = true;
    float cmap_opacity = 0.4;
    float point_size = 2.0f;

    GLint u_vertexOpacity_loc = -1;
    GLint u_projection_loc = -1;
    GLint u_view_loc = -1;
    GLint u_model_loc = -1;

    // Persistent mapped buffer
    float* mapped_buffer = nullptr;
    size_t buffer_size = 0;

    // 3D Camera State
    glm::vec3 cameraPos;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    float yaw;
    float pitch;
    float zoom;
    
    // Mouse dragging state for ImGui window
    bool isDragging = false;
    ImVec2 lastMousePos;
    
    void updateCameraVectors();
};
