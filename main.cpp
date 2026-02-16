#include <cstdint>
#include <memory>

#include <unistd.h> // getopt

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1


#include "imgui.h"
#include "imgui_elements.h"

#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers

#include "artfactory.h"
#include "gl_debug.h"
#include "gl_state.h"

std::unique_ptr<Art> art;

static GLFWwindow* window;
static int sw = 1024, sh = 1024;

bool get_window_size() {
    int _w, _h;
    glfwGetFramebufferSize(window, &_w, &_h);
    bool resized = (_w != sw || _h != sh);
    sw = _w; sh = _h;

    return resized;
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

char info[1024*4] = "Initializing...";

int main(int argc, char *argv[])
{
    unsigned frate = 0;

    int opt;
    int vsync = 1;
    int artarg = -1;
    bool shuffle_mode = false;
    bool save_every_frame = false;
    const char *title = "Dear ImGui screensaver";

    while ((opt = getopt(argc, argv, "sa:St:w")) != -1) {
        switch (opt) {
        case 's':
            vsync = 0;
            break;
        case 'a':
            artarg = atoi(optarg);
            break;
        case 'S':
            shuffle_mode = true;
            break;
        case 't':
            title = optarg;
            break;
        case 'w':
            save_every_frame = true;
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-s] [-a art_number] [-S] [-w]\n\n"
                "-s           disable vsync\n"
                "-a num       select screensaver\n"
                "-S           enable perioding shuffling of parameters (screensaver mode)\n"
                "-t title     set window title\n"
                "-w           save every frame to PNG",
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

    // Request debug context for OpenGL 4.3+ debug output
#ifndef NDEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    window = glfwCreateWindow(sw, sh, title, NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(vsync);

    // Initialize OpenGL debug callbacks (only effective in debug builds with OpenGL 4.3+)
    init_gl_debug();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    //init_shaders();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable( GL_BLEND );

    ImVec4 clear_color = ImVec4(0, 0, 0, 1.00f);

    ArtFactory af;
    if (artarg != -1)
        af.set_art(artarg);
    art = af.get_art();

    get_window_size();
    art->resized(sw, sh);
    //make_pbos();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        ImGui::NewFrame();


        ImGui::Begin(art->name());

        if (af.render_gui())
        {
            art = af.get_art();
            art->resized(sw, sh);
        }

        if (ImGui::CollapsingHeader("Clear Configuration"))
        {
            ScrollableSliderUInt("force clear every N frames", &art->clear_every, 0, 1024, "%d", 1);
            ScrollableSliderUInt("Max 1k frames before reinit", &art->max_kframes, 0, 1024, "%d", 1);
            ImGui::ColorEdit4("Clear color", (float*)&clear_color);
        }

        ImGui::Checkbox("", &save_every_frame);
        ImGui::SameLine();
        bool save_frame = ImGui::Button("Save Frame");

        art->gui();

        if (art->frame_number % 120 == 0)
        {
            char *ti = info;
            ti += cpu_load_text_now(info);
            ti += sprintf(ti, "\n%.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);

            if (shuffle_mode)
                art->check_shuffle(glfwGetTime());
            
        }
        ImGui::Text(info);

        ImGui::End();

        if (get_window_size()) {
            art->resized(sw, sh);
            glViewport(0, 0, sw, sh);
        }

        {
            GL_ENABLE_FOR_SCOPE(GL_BLEND);
            art->draw();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F12) || save_frame || save_every_frame) {
            // TODO: works only with EaselVertex and width/height is always like fullscreen
            art->save_frame();
        }

        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        if (art->clear_every != 0 && art->frame_number % art->clear_every == 0)
            art->clear();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();


    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
