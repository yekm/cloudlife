#include "easelvertex.h"

#include "imgui_elements.h"

#include <stdio.h>
#include <iostream>


const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    void main()
    {
        gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 fragColor;
    uniform vec4 vertexColor;
    void main()
    {
        fragColor = vertexColor;
    }
)";


void EaselVertex::init_shaders() {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
}

void EaselVertex::destroy_vertex_buffer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

EaselVertex::EaselVertex()
{
    init_shaders();
    create_vertex_buffer();
}


EaselVertex::~EaselVertex() {
    destroy_vertex_buffer();
    glDeleteProgram(shaderProgram);
}


void EaselVertex::render() {
    glUseProgram(shaderProgram);
    int vertexColorLocation = glGetUniformLocation(shaderProgram, "vertexColor");
    glUniform4f(vertexColorLocation, vertex_color.x, vertex_color.y, vertex_color.z, vertex_color.w);


    unsigned maxv = vertex_buffer_maximum();
    if (m_vertices.size() > 0) {
        unsigned offset = total_vertices;
        if (offset > maxv) {
            offset = total_vertices % maxv;
        }
        unsigned offset_n = offset - m_vertices.size() / 2;
        unsigned offset_b = offset_n * 2 * sizeof(float);
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
    ImGui::Text("vertices buffer size %d MiB", vertex_buffer_maximum()*2*sizeof(float)/1024/1024);
}

void EaselVertex::dab(float x, float y) {
    m_vertices.push_back(x);
    m_vertices.push_back(y);
    //m_vertices.push_back(0);
    ++total_vertices;
    // TODO: properly handle total_vertices overflow
    if (total_vertices > vertex_buffer_maximum()*2)
        total_vertices -= vertex_buffer_maximum();
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
