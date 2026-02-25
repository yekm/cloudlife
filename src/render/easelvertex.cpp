#include "easelvertex.h"

#include "imgui.h"
#include "imgui_elements.h"

#include <stdio.h>
#include <iostream>
#include <cstring>


const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in float aIndex;
    out float aIdx;
    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
        aIdx = aIndex;
    }
)";

const char* fragmentShaderSourceUniform = R"(
    #version 330 core
    in float aIdx;
    out vec4 fragColor;
    uniform vec4 vertexColor;
    void main()
    {
        fragColor = vertexColor;
    }
)";

const char* fragmentShaderSourceColormap = R"(
    #version 330 core
    in float aIdx;
    out vec4 fragColor;

    uniform float vertexOpacity;

    vec4 colormap(float x);

    void main()
    {
        vec4 cmc = colormap(aIdx);
        fragColor = vec4(cmc.x, cmc.y, cmc.z, vertexOpacity);
    }
)";


void EaselVertex::init_shaders() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    int success;
    char infoLog[512];

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        glDeleteShader(vertexShader);
        return;
    }

    // std::cout << fragmentShaderSource << std::endl;
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char * fss = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fss, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
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

    // Cache uniform locations to avoid querying every frame
    u_vertexOpacity_loc = glGetUniformLocation(shaderProgram, "vertexOpacity");
    u_vertexColor_loc = glGetUniformLocation(shaderProgram, "vertexColor");
}


void EaselVertex::create_vertex_buffer() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create and bind the vertex buffer object (VBO)
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Calculate buffer size
    buffer_size = vertex_buffer_maximum() * 3 * sizeof(float);

    glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_DYNAMIC_DRAW);
    
    cpu_backing_buffer.resize(buffer_size / sizeof(float));

    // Specify the vertex attribute pointers
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(float)));

    update_opacity = true;
}

void EaselVertex::destroy_vertex_buffer() {
    cpu_backing_buffer.clear();
    cpu_backing_buffer.shrink_to_fit();

    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

EaselVertex::EaselVertex()
{
    fragmentShaderSource = fragmentShaderSourceColormap + pal.get_cmap().getSource();
    //fragmentShaderSource = fragmentShaderSourceUniform;
    init_shaders();
    create_vertex_buffer();
    m_vertices.reserve(vertex_buffer_maximum()*3);
}


EaselVertex::~EaselVertex() {
    destroy_vertex_buffer();
    glDeleteProgram(shaderProgram);
}


void EaselVertex::render() {
    const unsigned maxv = vertex_buffer_maximum();

    glUseProgram(shaderProgram);

    if (use_colormap) {
        // Use cached uniform location - only update when GUI changes
        if (update_opacity && u_vertexOpacity_loc >= 0)
        {
            glUniform1f(u_vertexOpacity_loc, cmap_opacity);
            update_opacity = false;
        }
    }
    else {
        // Use cached uniform location
        if (u_vertexColor_loc >= 0) {
            glUniform4f(u_vertexColor_loc, vertex_color.x, vertex_color.y, vertex_color.z, vertex_color.w);
        }
    }


    // Copy vertex data directly to persistently mapped buffer
    if (m_vertices.size() > 0 && cpu_backing_buffer.size() > 0) {
        unsigned num_new_vertices = m_vertices.size() / 3;

        // Clamp to buffer capacity to prevent overflow
        if (num_new_vertices > maxv) {
            num_new_vertices = maxv;
        }

        // Calculate write position (end of current data, modulo buffer size)
        unsigned write_end = total_vertices % maxv;

        // Handle buffer wrap-around
        if (write_end < num_new_vertices) {
            // Write position wraps: split into tail + head
            unsigned tail_count = write_end;                    // Vertices to end of buffer
            unsigned head_count = num_new_vertices - write_end; // Vertices from start

            // Copy tail portion to end of buffer
            if (tail_count > 0) {
                memcpy(cpu_backing_buffer.data() + (maxv - tail_count) * 3,
                       m_vertices.data(),
                       tail_count * 3 * sizeof(float));
            }

            // Copy head portion to beginning of buffer
            if (head_count > 0) {
                memcpy(cpu_backing_buffer.data(),
                       m_vertices.data() + tail_count * 3,
                       head_count * 3 * sizeof(float));
            }
        } else {
            // Normal case: contiguous write
            unsigned write_start = write_end - num_new_vertices;
            memcpy(cpu_backing_buffer.data() + write_start * 3,
                   m_vertices.data(),
                   num_new_vertices * 3 * sizeof(float));
        }

        // Upload the updated data to the GPU buffer
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        if (write_end < num_new_vertices) {
            // Write position wraps: split upload into tail + head
            unsigned tail_count = write_end;
            unsigned head_count = num_new_vertices - write_end;

            if (tail_count > 0) {
                glBufferSubData(GL_ARRAY_BUFFER, 
                                (maxv - tail_count) * 3 * sizeof(float), 
                                tail_count * 3 * sizeof(float), 
                                cpu_backing_buffer.data() + (maxv - tail_count) * 3);
            }
            if (head_count > 0) {
                glBufferSubData(GL_ARRAY_BUFFER, 
                                0, 
                                head_count * 3 * sizeof(float), 
                                cpu_backing_buffer.data());
            }
        } else {
            // Normal case: contiguous write
            unsigned write_start = write_end - num_new_vertices;
            glBufferSubData(GL_ARRAY_BUFFER, 
                            write_start * 3 * sizeof(float), 
                            num_new_vertices * 3 * sizeof(float), 
                            cpu_backing_buffer.data() + write_start * 3);
        }
    }

    glBindVertexArray(vao);
    auto to_draw = total_vertices>maxv ? maxv : total_vertices;
    glDrawArrays(GL_POINTS, 0, to_draw);
    glBindVertexArray(0);

    m_vertices.clear();
}

void EaselVertex::gui() {
    ImGui::ColorEdit4("Vertex color", (float*)&vertex_color);
    if (ScrollableSliderUInt("Max vertex", &vertex_buffer_maximum_k, 1024, 1024*32, "%d", 32)) {
        destroy_vertex_buffer();
        create_vertex_buffer();
    }
    if (ScrollableSliderUInt("Frame vertex target", &frame_vertex_target_k, 1, vertex_buffer_maximum_k, "%d", 8))
        m_vertices.reserve(vertex_buffer_maximum()*3);

    //ImGui::Checkbox("Use colormap", &use_colormap);
    if (use_colormap) {
        if (pal.RenderGui()) {
            fragmentShaderSource = fragmentShaderSourceColormap + pal.get_cmap().getSource();
            destroy_vertex_buffer();
            glDeleteProgram(shaderProgram);
            init_shaders();
            create_vertex_buffer();
        }
        update_opacity |= ScrollableSliderFloat("Opacity", &cmap_opacity, 0, 1, "%.2f", 0.02);
    }

    ImGui::Text("vertices buffer size %ld MiB", vertex_buffer_maximum()*3*sizeof(float)/1024/1024);
    ImGui::Text("total vertices %d, m_vertices.size() %ld", total_vertices, m_vertices.size());
}

void EaselVertex::dab(float x, float y) {
    const auto vbm = vertex_buffer_maximum();
    //unsigned index = (m_vertices.size()/3);// % frame_vertex_target();
    m_vertices.push_back(x);
    m_vertices.push_back(y);
    m_vertices.push_back(pal.get_next_color_index());
    //m_vertices.push_back(0);
    ++total_vertices;
    // TODO: properly handle total_vertices overflow
    if (total_vertices > vbm*2)
        total_vertices -= vbm;
}

/*
void EaselVertex::dab(uint32_t x, uint32_t y, uint32_t c) {
    dab((float)x, (float)y);
}
*/

void EaselVertex::drawdot(int32_t x, int32_t y, uint32_t c) {
    dab((float)x/w-0.5, (float)y/h-0.5);
}

void EaselVertex::drawdot(float x, float y) {
    dab((float)x/w-0.5, (float)y/h-0.5);
}

void EaselVertex::clear() {
    total_vertices = 0;
}

/*
unsigned EaselVertex::vertex_buffer_maximum() const {
    return vertex_buffer_maximum_k * 1024;
}
*/
