#include <vector>
#include <stdint.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "vertexbuffer.h"

#include <stdio.h>

VertexBuffer::VertexBuffer(unsigned max_vertices)
    : m_max_vertices(max_vertices)
{

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create and bind the vertex buffer object (VBO)
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);


    glBufferData(GL_ARRAY_BUFFER, m_max_vertices*3*sizeof(float), 0, GL_STATIC_DRAW);
    // Specify the vertex attribute pointers
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

}

VertexBuffer::~VertexBuffer() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);

}

void VertexBuffer::draw() {
    unsigned offset = total_vertices;
    if (offset > m_max_vertices) {
        offset = total_vertices%m_max_vertices;
    }
    unsigned offset_n = offset - m_vertices.size()/3;
    unsigned offset_b = offset_n*3*sizeof(float);
    //printf("[%d %d]", offset, m_vertices.size()/3);
    //printf("%.2f_%.2f ", m_vertices.at(0), m_vertices.at(1));
    //fflush(stdout);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, offset_b, m_vertices.size()*sizeof(float), m_vertices.data());

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, total_vertices>m_max_vertices?m_max_vertices:total_vertices);
    glBindVertexArray(0);

    m_vertices.clear();
}

void VertexBuffer::append(Pixel && p) {
    m_vertices.push_back(p.x);
    m_vertices.push_back(p.y);
    m_vertices.push_back(0);
    //buffer.emplace_back(std::move(p));
    ++total_vertices;
}

void VertexBuffer::adot(float x, float y) {
    m_vertices.push_back(x);
    m_vertices.push_back(y);
    m_vertices.push_back(0);
    //buffer.emplace_back(std::move(p));
    ++total_vertices;
}

void VertexBuffer::clear() {
    total_vertices = 0;
}
