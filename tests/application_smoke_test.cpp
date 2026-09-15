#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>
#include <unordered_set>

namespace {
std::unordered_set<GLuint> live_buffers;
PFNGLGENBUFFERSPROC real_gen_buffers = nullptr;
PFNGLDELETEBUFFERSPROC real_delete_buffers = nullptr;
bool lifetime_failed = false;
bool window_created = false;
unsigned frames = 0;

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
    char* arguments[] = {program, select, argc > 1 ? argv[1] : default_art, hide, nullptr};
    const int result = cloudlife_main(4, arguments);
    if (!window_created)
        return 77;
    if (result != 0 || frames != 3 || lifetime_failed)
        return 1;
    std::puts("Application released art buffers before destroying the OpenGL context.");
}
