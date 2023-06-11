#include <cstdint>
#include <memory>

#include <unistd.h> // getopt
#include <error.h>

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1


#include "imgui.h"
#include "imgui_elements.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "artfactory.h"

std::unique_ptr<Art> art;


static void
*xrealloc(void *p, size_t size)
{
    void *ret;
    if ((ret = realloc(p, size)) == NULL) {
        error(1, errno, "out of memory");
    }
    return ret;
}

static GLFWwindow* window;
static int tex_w, tex_h, texture_size;
static int sw = 1024, sh = 1024;


bool get_window_size(int *w, int *h) {
    bool ret = false;
    int _w, _h;
    glfwGetFramebufferSize(window, &_w, &_h);
    if (_w != sw || _h != sh) {
        ret = true;
        if (w) *w = _w;
        if (h) *h = _h;
    }
    sw = _w; sh = _h;

    return ret;
}

uint32_t *image_data = NULL;

GLuint image_texture;
GLuint pboIds[2];
int pbo_index = 0;

ImVec4 clear_color = ImVec4(0, 0, 0, 1.00f);

void make_pbos() {
    if (!art->override_texture_size(tex_w, tex_h))
        tex_w = sw, tex_h = sh;
    texture_size = tex_w * tex_h * 4;
    image_data = (uint32_t*)xrealloc(image_data, texture_size);

    // AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
    // http://www.songho.ca/opengl/gl_pbo.html
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D,
            0, GL_RGBA, tex_w, tex_h,
            0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(2, pboIds);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size,
            0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size,
            0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void destroy_pbos() {
    glDeleteTextures(1, &image_texture);
    glDeleteBuffers(2, pboIds);
}


static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}


int main(int argc, char *argv[])
{
    int opt;
    int vsync = 1;

    while ((opt = getopt(argc, argv, "s")) != -1) {
        switch (opt) {
        case 's':
            vsync = 0;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-s]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }


    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif

    window = glfwCreateWindow(sw, sh, "Dear ImGui screensaver", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(vsync);


    ArtFactory af;
    art = af.get_art();


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    get_window_size(0,0);
    art->resized(sw, sh);
    make_pbos();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGui::Begin(art->name());

        if (af.render_gui())
        {
            art = af.get_art();
            destroy_pbos();
            make_pbos();
            art->resized(sw, sh);
       }

        ImGui::ColorEdit4("Clear color", (float*)&clear_color);
        ScrollableSliderUInt("force clear every N frames", &art->clear_every, 0, 1024, "%d", 2);

        bool reinit = art->gui();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

        cpu_load_text();

        ImGui::End();

        if (get_window_size(0,0)) {
            destroy_pbos();
            make_pbos();
            art->resized(sw, sh);
        }

        int nexti = pbo_index;
        pbo_index = pbo_index ? 0 : 1;
        glBindTexture(GL_TEXTURE_2D, image_texture);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[pbo_index]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                tex_w, tex_h, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nexti]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, texture_size,
                0, GL_STREAM_DRAW);
        uint32_t* ptr = (uint32_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        assert(ptr);
        art->draw(ptr);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        //ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)image_texture,
        //    ImVec2(0, 0), ImVec2(tex_w, tex_h));

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)image_texture,
            ImVec2(0, 0), ImVec2(display_w, display_h),
            ImVec2(0, 0), ImVec2((float)display_w/tex_w, (float)display_h/tex_h));

        ImGui::Render();
        //glViewport(0, 0, sw, sh);
        glViewport(0, 0, display_w, display_h);

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        ++art->frame_number;
        if (art->clear_every != 0 && art->frame_number % art->clear_every == 0)
            art->clear();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    destroy_pbos();
    free(image_data);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
