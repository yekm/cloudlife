#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <unordered_set>
#include <vector>

namespace {
std::unordered_set<GLuint> live_buffers;
PFNGLGENBUFFERSPROC real_gen_buffers = nullptr;
PFNGLDELETEBUFFERSPROC real_delete_buffers = nullptr;
PFNGLREADPIXELSPROC real_read_pixels = nullptr;
bool lifetime_failed = false;
bool window_created = false;
bool capture_frames = false;
bool capture_failed = false;
bool colored_frame = false;
unsigned captures = 0;
unsigned frames = 0;
std::vector<unsigned char> captured_pixels;

void APIENTRY track_read_pixels(GLint x, GLint y, GLsizei width, GLsizei height,
                               GLenum format, GLenum type, void* pixels)
{
    real_read_pixels(x, y, width, height, format, type, pixels);
    if (capture_frames && format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
        const auto* bytes = static_cast<const unsigned char*>(pixels);
        captured_pixels.assign(bytes, bytes + static_cast<size_t>(width) * height * 4);
        ++captures;
    }
}

void APIENTRY track_gen_buffers(GLsizei count, GLuint* buffers)
{
    real_gen_buffers(count, buffers);
    for (GLsizei i = 0; i < count; ++i)
        live_buffers.insert(buffers[i]);
}

void APIENTRY track_delete_buffers(GLsizei count, const GLuint* buffers)
{
    if (!glfwGetCurrentContext()) {
        lifetime_failed = true;
        return;
    }
    for (GLsizei i = 0; i < count; ++i)
        live_buffers.erase(buffers[i]);
    real_delete_buffers(count, buffers);
}

int load_tracked_gl(GLADloadproc load)
{
    const int result = gladLoadGLLoader(load);
    if (result) {
        real_gen_buffers = glad_glGenBuffers;
        real_delete_buffers = glad_glDeleteBuffers;
        glad_glGenBuffers = track_gen_buffers;
        glad_glDeleteBuffers = track_delete_buffers;
        real_read_pixels = glad_glReadPixels;
        glad_glReadPixels = track_read_pixels;
    }
    return result;
}

GLFWwindow* create_test_window(int, int, const char* title, GLFWmonitor* monitor, GLFWwindow* share)
{
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* result = glfwCreateWindow(64, 64, title, monitor, share);
    window_created = result != nullptr;
    return result;
}

void destroy_test_window(GLFWwindow* window)
{
    if (!live_buffers.empty()) {
        std::fprintf(stderr, "%zu art buffers still live at context destruction\n", live_buffers.size());
        lifetime_failed = true;
    }
    glfwDestroyWindow(window);
}

void swap_test_buffers(GLFWwindow* window)
{
    if (capture_frames) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        std::vector<unsigned char> presented_pixels(static_cast<size_t>(width) * height * 4);
        real_read_pixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, presented_pixels.data());
        if (captures != frames + 1 || captured_pixels != presented_pixels)
            capture_failed = true;
        for (size_t i = 0; i < presented_pixels.size(); i += 4) {
            if (presented_pixels[i] || presented_pixels[i + 1] || presented_pixels[i + 2])
                colored_frame = true;
        }
    }
    glfwSwapBuffers(window);
    if (++frames == 3)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}
}

// Exercise the actual application entry point with a hidden, bounded window.
#define main cloudlife_main
#define gladLoadGLLoader load_tracked_gl
#define glfwCreateWindow create_test_window
#define glfwDestroyWindow destroy_test_window
#define glfwSwapBuffers swap_test_buffers
#include "../main.cpp"
#undef glfwSwapBuffers
#undef glfwDestroyWindow
#undef glfwCreateWindow
#undef gladLoadGLLoader
#undef main

int main(int argc, char** argv)
{
    char program[] = "cloudlife-test";
    char select[] = "-a";
    char default_art[] = "1";
    char hide[] = "-g";
    char write[] = "-w";
    capture_frames = argc > 2;
    const bool show_gui = capture_frames && std::strcmp(argv[2], "--capture-gui") == 0;
    char* arguments[] = {program, select, argc > 1 ? argv[1] : default_art,
                         show_gui ? write : hide, capture_frames && !show_gui ? write : nullptr, nullptr};

    const auto original_directory = std::filesystem::current_path();
    std::filesystem::path capture_directory;
    if (capture_frames) {
        capture_directory = std::filesystem::temp_directory_path() /
            ("cloudlife-capture-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(capture_directory);
        std::filesystem::current_path(capture_directory);
    }
    const int result = cloudlife_main(capture_frames && !show_gui ? 5 : 4, arguments);
    if (capture_frames) {
        unsigned png_count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(capture_directory)) {
            if (entry.path().extension() == ".png" && entry.file_size() > 0)
                ++png_count;
        }
        capture_failed |= png_count != 3 || !colored_frame;
        std::filesystem::current_path(original_directory);
        std::filesystem::remove_all(capture_directory);
    }
    if (!window_created)
        return 77;
    const int selected_id = argc > 1 ? std::atoi(argv[1]) : 1;
    if (((selected_id == 0 || selected_id == 12) && !GLAD_GL_VERSION_3_3) ||
        (selected_id >= 17 && !GLAD_GL_VERSION_4_3)) {
        std::puts("Requested renderer is unsupported by this test context; skipping.");
        return 77;
    }
    if (result != 0 || frames != 3 || lifetime_failed || capture_failed) {
        if (capture_failed)
            std::fprintf(stderr, "Screenshot pixels did not match the completed nonempty frame or PNGs were missing\n");
        return 1;
    }
    std::puts("Application released art buffers before destroying the OpenGL context.");
}
