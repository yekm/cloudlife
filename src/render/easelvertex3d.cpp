#include "easelvertex3d.h"

#include "imgui.h"
#include "imgui_elements.h"

#include "gl_state.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <utility>

const char* vertexShaderSource3D = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in float aIndex;
    out float aIdx;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main()
    {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
        aIdx = aIndex;
    }
)";

const char* fragmentShaderSourceUniform3D = R"(
    #version 330 core
    in float aIdx;
    out vec4 fragColor;
    uniform vec4 vertexColor;
    void main()
    {
        fragColor = vertexColor;
    }
)";

EaselVertex3D::EaselVertex3D()
    : Easel()
{
    cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
    cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
    
    yaw = -90.0f;
    pitch = 0.0f;
    zoom = 45.0f;
    
    updateCameraVectors();

    frame_vertex_target_k = 64;

    build_fragment_shader_source();
    create_vertex_buffer();
    init_shaders();
}

EaselVertex3D::~EaselVertex3D() {
    destroy_vertex_buffer();
    glDeleteProgram(shaderProgram);
}

void EaselVertex3D::create_vertex_buffer() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // x, y, z coords
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // color index
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void EaselVertex3D::destroy_vertex_buffer() {
    cpu_backing_buffer.clear();
    cpu_backing_buffer.shrink_to_fit();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

void EaselVertex3D::ensure_cpu_capacity(size_t required_floats) {
    if (required_floats <= cpu_backing_buffer.size())
        return;

    if (required_floats > cpu_backing_buffer.capacity()) {
        const size_t current_capacity = cpu_backing_buffer.capacity();
        const size_t grown_capacity = current_capacity > 0 ? current_capacity * 2 : 4096;
        cpu_backing_buffer.reserve(std::max(required_floats, grown_capacity));
    }
    cpu_backing_buffer.resize(required_floats);
}

void EaselVertex3D::ensure_gpu_capacity(size_t required_bytes) {
    if (required_bytes <= gpu_buffer_size)
        return;

    const size_t grown_capacity = gpu_buffer_size > 0 ? gpu_buffer_size * 2 : 4096 * sizeof(float);
    gpu_buffer_size = std::max(required_bytes, grown_capacity);
}

void EaselVertex3D::build_fragment_shader_source() {
    std::string cmapSource = pal.get_cmap().getSource();
    fragmentShaderSource = std::string(R"(
        #version 330 core
        in float aIdx;
        out vec4 fragColor;
        uniform float vertexOpacity;
        vec4 colormap(float x);
    )") + cmapSource + R"(
        void main()
        {
            vec4 color = colormap(aIdx);
            color.a *= vertexOpacity;
            fragColor = color;
        }
    )";
}

void EaselVertex3D::init_shaders() {
    unsigned vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource3D, NULL);
    glCompileShader(vertexShader);

    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return;
    }

    unsigned fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char * src = use_colormap ? fragmentShaderSource.c_str() : fragmentShaderSourceUniform3D;
    glShaderSource(fragmentShader, 1, &src, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    u_vertexOpacity_loc = glGetUniformLocation(shaderProgram, "vertexOpacity");
    u_projection_loc = glGetUniformLocation(shaderProgram, "projection");
    u_view_loc = glGetUniformLocation(shaderProgram, "view");
    u_model_loc = glGetUniformLocation(shaderProgram, "model");
}

void EaselVertex3D::reload_shaders() {
    glDeleteProgram(shaderProgram);
    init_shaders();
}

void EaselVertex3D::drawdot(int32_t x, int32_t y, uint32_t c) {
    // Unsupported for 3D natively, map to 0 Z
    drawdot((float)x, (float)y, 0.0f, (float)(c & 0xff));
}

void EaselVertex3D::drawdot(float x, float y, float z, float c) {
    if (geometry_frozen) return;
    if (total_vertices >= vertex_buffer_maximum()) return;

    size_t index = total_vertices * 4;
    ensure_cpu_capacity(index + 4);
    cpu_backing_buffer[index]     = x;
    cpu_backing_buffer[index + 1] = y;
    cpu_backing_buffer[index + 2] = z;
    cpu_backing_buffer[index + 3] = c;
    
    total_vertices++;
    geometry_dirty = true;
}

bool EaselVertex3D::replace_geometry(std::vector<float> vertices) {
    if (vertices.size() % 4 != 0) {
        std::fprintf(stderr, "EaselVertex3D: packed geometry must contain x, y, z, color values\n");
        return false;
    }

    const size_t vertex_count = vertices.size() / 4;
    if (vertex_count > vertex_buffer_maximum()) {
        std::fprintf(stderr, "EaselVertex3D: geometry contains %zu vertices; maximum is %u\n",
                     vertex_count, vertex_buffer_maximum());
        return false;
    }

    cpu_backing_buffer = std::move(vertices);
    total_vertices = static_cast<unsigned>(vertex_count);
    frozen_vertices = total_vertices;
    geometry_frozen = true;
    geometry_dirty = true;
    return true;
}

void EaselVertex3D::freeze_geometry() {
    frozen_vertices = total_vertices;
    geometry_frozen = true;
    geometry_dirty = true;
}

void EaselVertex3D::set_model_matrix(const glm::mat4& model) {
    model_matrix = model;
}

void EaselVertex3D::clear() {
    total_vertices = 0;
    frozen_vertices = 0;
    geometry_frozen = false;
    geometry_dirty = true;
}

void EaselVertex3D::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
    
    // Recalculate Right and Up vector
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));  
    cameraUp    = glm::normalize(glm::cross(right, cameraFront));
}

void EaselVertex3D::gui() {
    if (ImGui::CollapsingHeader("EaselVertex3D Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Vertices: %u / %u", total_vertices, vertex_buffer_maximum());
        ScrollableSliderFloat("Point Size", &point_size, 0.1f, 10.0f, "%.1f", 0.1f);
        
        ImGui::Separator();
        ImGui::Text("Camera Controls");
        ImGui::DragFloat3("Position", glm::value_ptr(cameraPos), 0.1f);
        if (ImGui::DragFloat("Yaw", &yaw, 1.0f)) updateCameraVectors();
        if (ImGui::DragFloat("Pitch", &pitch, 1.0f, -89.0f, 89.0f)) updateCameraVectors();
        ScrollableSliderFloat("Zoom (FOV)", &zoom, 1.0f, 120.0f, "%.1f", 1.0f);
        
        ImGui::Separator();
        ImGui::Text("Colormap Options");
        if (pal.RenderGui()) {
            build_fragment_shader_source();
            reload_shaders();
        }
        ScrollableSliderFloat("Opacity", &cmap_opacity, 0.0f, 1.0f, "%.2f", 0.02f);
        
        if (ImGui::Button("Reset Camera")) {
            cameraPos   = glm::vec3(0.0f, 0.0f,  3.0f);
            cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
            cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
            yaw = -90.0f;
            pitch = 0.0f;
            zoom = 45.0f;
            updateCameraVectors();
        }

        ImGui::Separator();
        ImGui::Text("Drag to rotate, Scroll to zoom");
        
        // Handle drag to rotate on the main window using io
        ImGuiIO& io = ImGui::GetIO();
        
        // Ensure ImGui does not capture the mouse (e.g. over a window)
        if (!io.WantCaptureMouse) {
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                isDragging = true;
                lastMousePos = io.MousePos;
            } else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                isDragging = false;
            }
            
            if (isDragging) {
                float xoffset = io.MousePos.x - lastMousePos.x;
                float yoffset = lastMousePos.y - io.MousePos.y; // reversed since y-coordinates range from bottom to top
                lastMousePos = io.MousePos;

                float sensitivity = 0.5f;
                xoffset *= sensitivity;
                yoffset *= sensitivity;

                yaw   += xoffset;
                pitch += yoffset;

                if (pitch > 89.0f)
                    pitch = 89.0f;
                if (pitch < -89.0f)
                    pitch = -89.0f;

                updateCameraVectors();
            }
            
            if (io.MouseWheel != 0.0f) {
                zoom -= io.MouseWheel * 2.0f;
                if (zoom < 1.0f) zoom = 1.0f;
                if (zoom > 120.0f) zoom = 120.0f;
            }
        }
    }
}

void EaselVertex3D::render() {
    if (total_vertices == 0) return;

    glUseProgram(shaderProgram);

    // Set matrices
    glm::mat4 projection = glm::perspective(glm::radians(zoom), (float)ww / (float)wh, 0.1f, 100.0f);
    glUniformMatrix4fv(u_projection_loc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(u_view_loc, 1, GL_FALSE, glm::value_ptr(view));
    
    glUniformMatrix4fv(u_model_loc, 1, GL_FALSE, glm::value_ptr(model_matrix));

    if (use_colormap) {
        glUniform1f(u_vertexOpacity_loc, cmap_opacity);
    }

    // Set temporary state for this render call using RAII wrappers from gl_state.h
    GL_ENABLE_FOR_SCOPE(GL_BLEND);
    GL_BLEND_FUNC_FOR_SCOPE(GL_SRC_ALPHA, GL_ONE);

    // RAII for point size doesn't exist, we must save and restore
    GLfloat old_point_size;
    glGetFloatv(GL_POINT_SIZE, &old_point_size);
    glPointSize(point_size);

    glBindVertexArray(vao);
    
    const unsigned vertices_to_draw = geometry_frozen ? frozen_vertices : total_vertices;

    // Orphan before every changed upload to avoid waiting for a draw that is still consuming
    // the previous contents. Frozen geometry is unchanged between these dirty uploads.
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    if (!geometry_frozen || geometry_dirty) {
        const size_t upload_size = static_cast<size_t>(vertices_to_draw) * 4 * sizeof(float);
        ensure_gpu_capacity(upload_size);
        glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(gpu_buffer_size), nullptr,
                     GL_DYNAMIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(upload_size),
            cpu_backing_buffer.data());
        geometry_dirty = false;
    }
    
    glDrawArrays(GL_POINTS, 0, vertices_to_draw);
    
    // Restore state
    glPointSize(old_point_size);

    if (!geometry_frozen)
        total_vertices = 0; // Dynamic geometry accumulates new points each frame.
}
