#include <vector>
#include <stdint.h>

class VertexBuffer {
public:
    struct Pixel {
        uint32_t x, y;
        uint32_t color;
    };

    VertexBuffer(unsigned max_vertices);
    ~VertexBuffer();

    void draw();

    void append(Pixel && p);
    void adot(float x, float y);

    void clear();

    unsigned vao, vbo;
    std::vector<float> m_vertices;
    unsigned total_vertices = 0, m_max_vertices;
};
