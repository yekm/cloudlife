#include "easelplane.h"

#include "imgui.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

struct Buffer {
    std::vector<uint32_t> pixels;
    bool mapped = false;
};

std::unordered_map<GLuint, Buffer> buffers;
std::unordered_set<GLuint> textures;
std::vector<uint32_t> texture;
GLuint next_id = 1;
GLuint bound_buffer = 0;
GLuint bound_texture = 0;
unsigned uploads = 0;
bool fail_map = false;
bool fail_unmap = false;

void APIENTRY gen_textures(GLsizei count, GLuint* ids)
{
    for (GLsizei i = 0; i < count; ++i) {
        ids[i] = next_id++;
        textures.insert(ids[i]);
    }
}

void APIENTRY bind_texture(GLenum, GLuint id)
{
    bound_texture = id;
}

void APIENTRY tex_parameter(GLenum, GLenum, GLint) {}

void APIENTRY tex_image(GLenum, GLint, GLint, GLsizei width, GLsizei height,
        GLint, GLenum, GLenum, const void* pixels)
{
    require(bound_buffer == 0, "initial texture data must be a CPU pointer");
    require(bound_texture != 0, "texture initialization needs a texture");
    const auto* source = static_cast<const uint32_t*>(pixels);
    texture.assign(source, source + width * height);
}

void APIENTRY gen_buffers(GLsizei count, GLuint* ids)
{
    for (GLsizei i = 0; i < count; ++i) {
        ids[i] = next_id++;
        buffers.emplace(ids[i], Buffer{});
    }
}

void APIENTRY bind_buffer(GLenum, GLuint id)
{
    require(id == 0 || buffers.count(id) == 1, "binding an unknown PBO");
    bound_buffer = id;
}

void APIENTRY buffer_data(GLenum, GLsizeiptr size, const void* pixels, GLenum)
{
    require(bound_buffer != 0, "buffer allocation needs a PBO");
    auto& buffer = buffers.at(bound_buffer);
    require(!buffer.mapped, "must unmap before replacing storage");
    require(size > 0 && size % sizeof(uint32_t) == 0, "PBO size must be in bytes");
    // Poison orphaned memory so missing initialization is observable.
    buffer.pixels.assign(size / sizeof(uint32_t), 0xdeadbeef);
    if (pixels)
        std::memcpy(buffer.pixels.data(), pixels, size);
}

void* APIENTRY map_buffer(GLenum, GLintptr offset, GLsizeiptr size, GLbitfield)
{
    auto& buffer = buffers.at(bound_buffer);
    require(!buffer.mapped, "must not map a PBO twice");
    require(offset == 0 && size == buffer.pixels.size() * sizeof(uint32_t),
            "mapping must cover the whole image");
    if (fail_map) {
        fail_map = false;
        return nullptr;
    }
    buffer.mapped = true;
    return buffer.pixels.data();
}

GLboolean APIENTRY unmap_buffer(GLenum)
{
    auto& buffer = buffers.at(bound_buffer);
    require(buffer.mapped, "unmap must target the mapped PBO");
    buffer.mapped = false;
    if (fail_unmap) {
        fail_unmap = false;
        std::fill(buffer.pixels.begin(), buffer.pixels.end(), 0xdeadbeef);
        return GL_FALSE;
    }
    return GL_TRUE;
}

void APIENTRY tex_sub_image(GLenum, GLint, GLint, GLint, GLsizei width,
        GLsizei height, GLenum, GLenum, const void* offset)
{
    require(bound_texture != 0, "upload needs a texture");
    require(bound_buffer != 0 && offset == nullptr, "upload must use a PBO");
    const auto& buffer = buffers.at(bound_buffer);
    require(!buffer.mapped, "texture upload must happen after unmap");
    require(buffer.pixels.size() == static_cast<size_t>(width * height),
            "PBO must contain exactly one image");
    texture = buffer.pixels;
    ++uploads;
}

void APIENTRY delete_textures(GLsizei count, const GLuint* ids)
{
    for (GLsizei i = 0; i < count; ++i) {
        if (ids[i] != 0)
            require(textures.erase(ids[i]) == 1, "texture deleted twice");
    }
}

void APIENTRY delete_buffers(GLsizei count, const GLuint* ids)
{
    for (GLsizei i = 0; i < count; ++i) {
        if (ids[i] == 0)
            continue;
        require(buffers.count(ids[i]) == 1, "PBO deleted twice");
        require(!buffers.at(ids[i]).mapped, "PBO still mapped at destruction");
        buffers.erase(ids[i]);
    }
}

GLenum APIENTRY get_error()
{
    return GL_NO_ERROR;
}

void expect_image(std::initializer_list<uint32_t> expected)
{
    require(texture == std::vector<uint32_t>(expected), "unexpected texture pixels");
    require(bound_buffer == 0, "upload must leave the unpack buffer unbound");
}

} // namespace

int main()
{
    glad_glGenTextures = gen_textures;
    glad_glBindTexture = bind_texture;
    glad_glTexParameteri = tex_parameter;
    glad_glTexImage2D = tex_image;
    glad_glGenBuffers = gen_buffers;
    glad_glBindBuffer = bind_buffer;
    glad_glBufferData = buffer_data;
    glad_glMapBufferRange = map_buffer;
    glad_glUnmapBuffer = unmap_buffer;
    glad_glTexSubImage2D = tex_sub_image;
    glad_glDeleteTextures = delete_textures;
    glad_glDeleteBuffers = delete_buffers;
    glad_glGetError = get_error;

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.DisplaySize = ImVec2(100, 100);
    unsigned char* font_pixels;
    int font_width, font_height;
    io.Fonts->GetTexDataAsRGBA32(&font_pixels, &font_width, &font_height);
    ImGui::NewFrame();

    unsigned uploads_before_destruction = 0;
    {
        EaselPlane plane;
        plane.set_window_size(2, 2);
        plane.set_texture_size(2, 2);
        expect_image({0, 0, 0, 0});

        plane.begin();
        plane.drawdot(0, 0, 1);
        plane.render();
        expect_image({1, 0, 0, 0});
        plane.begin();
        plane.drawdot(1, 1, 2);
        plane.render();
        expect_image({1, 0, 0, 2});
        plane.begin();
        plane.drawdot(1, 0, 3);
        plane.render();
        expect_image({1, 3, 0, 2});

        fail_map = true;
        plane.begin();
        plane.drawdot(0, 1, 4);
        plane.render();
        expect_image({1, 3, 4, 2});
        fail_unmap = true;
        plane.begin();
        plane.drawdot(1, 1, 5);
        plane.render();
        expect_image({1, 3, 4, 5});

        plane.begin();
        plane.clear1();
        plane.drawdot(0, 0, 6);
        plane.render();
        expect_image({6, 0, 0, 0});
        plane.begin();
        plane.clear();
        expect_image({0, 0, 0, 0});
        plane.drawdot(1, 1, 7);
        plane.render();
        expect_image({0, 0, 0, 7});
        plane.begin();
        plane.render();
        expect_image({0, 0, 0, 7});

        plane.set_texture_size(2, 2);
        expect_image({0, 0, 0, 0});
        plane.set_texture_size(3, 1);
        plane.drawdot(-1, 0, 8);
        plane.drawdot(3, 0, 8);
        plane.drawdot(0, 1, 8);
        plane.drawdot(2, 0, 9);
        plane.render();
        expect_image({0, 0, 9});
        plane.set_texture_size(0, 0);
        require(buffers.empty(), "zero-size reset must destroy PBOs");
        require(textures.empty(), "zero-size reset must destroy textures");
        plane.begin();
        plane.clear();
        plane.render();
        plane.set_texture_size(1, 1);
        plane.begin();
        plane.drawdot(0, 0, 10);
        plane.render();
        expect_image({10});
        plane.begin();
        uploads_before_destruction = uploads;
    }
    require(buffers.empty(), "destruction must release PBOs");
    require(textures.empty(), "destruction must release textures");
    require(uploads == uploads_before_destruction, "destruction must not upload images");
    ImGui::EndFrame();
    ImGui::DestroyContext();
    puts("EaselPlane upload and persistence checks passed");
}
