#include "easelvertex.h"

#include "imgui.h"
#include "imgui_elements.h"

#include <stdio.h>
#include <iostream>


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
    }


    std::cout << fragmentShaderSource << std::endl;
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char * fss = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fss, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
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
}


void EaselVertex::create_vertex_buffer() {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create and bind the vertex buffer object (VBO)
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);


    glBufferData(GL_ARRAY_BUFFER, vertex_buffer_maximum()*2*sizeof(float), 0, GL_STATIC_DRAW);
    // Specify the vertex attribute pointers
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(float)));

    update_opacity = true;
}

void EaselVertex::destroy_vertex_buffer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

EaselVertex::EaselVertex()
{
    fragmentShaderSource = fragmentShaderSourceColormap + pal.get_cmap().getSource();
    //fragmentShaderSource = fragmentShaderSourceUniform;
    init_shaders();
    create_vertex_buffer();
}


EaselVertex::~EaselVertex() {
    destroy_vertex_buffer();
    glDeleteProgram(shaderProgram);
}


void EaselVertex::render() {
    const unsigned maxv = vertex_buffer_maximum();

    glUseProgram(shaderProgram);

    if (use_colormap) {
        // there is no need to execute this every frame
        if (update_opacity) // requested by gui
        {
            int opacityColorLocation = glGetUniformLocation(shaderProgram, "vertexOpacity");
            glUniform1f(opacityColorLocation, cmap_opacity);
            update_opacity = false;
        }
    }
    else {
        int vertexColorLocation = glGetUniformLocation(shaderProgram, "vertexColor");
        glUniform4f(vertexColorLocation, vertex_color.x, vertex_color.y, vertex_color.z, vertex_color.w);
    }


    if (m_vertices.size() > 0) {
        unsigned offset = total_vertices;
        if (offset > maxv) {
            offset = total_vertices % maxv;
        }
        unsigned offset_n = offset - m_vertices.size() / 3;
        unsigned offset_b = offset_n * 3 * sizeof(float);
        //printf("[%d %d]", offset, m_vertices.size()/2);
        //printf("%.2f_%.2f ", m_vertices.at(0), m_vertices.at(1));
        //fflush(stdout);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, offset_b, m_vertices.size()*sizeof(float), m_vertices.data());
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
    ScrollableSliderUInt("Frame vertex target", &frame_vertex_target_k, 1, vertex_buffer_maximum_k, "%d", 8);

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

    ImGui::Text("vertices buffer size %d MiB", vertex_buffer_maximum()*3*sizeof(float)/1024/1024);
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
