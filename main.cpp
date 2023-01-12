#include <cstdint>
#include <deque>
#include <bitset>
#include <iostream>

#include <math.h> // exp


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
//#include <GL/glut.h>

struct odot {
    int ax, ay, bx, by, cx, cy;
};

std::deque<odot> osc;

int maxodots = 1024*6;
int filler_sleep = 100;
int maxdots_perframe = 64;
//float gm = -2.5;
float gm = 3.5;
int dots_clamped = 64;
ImVec4 ocolor1 = ImVec4(1, 0, 0, 0);
ImVec4 ocolor2 = ImVec4(0, 1, 0, 0);
ImVec4 ocolor3 = ImVec4(0, 0, 1, 0);

#define TEX_W 1024
#define TEX_H 1024

uint32_t image_data[TEX_W*TEX_H];


//unsigned int tb = 0b011000111001110011100010000010;
  unsigned int tb = 0b001100011100111001110001000001; // original
//unsigned int tb = 0b001110011100111001110001000001;
//unsigned int tb = 0b000110001100111001110001000010; // alt1
//unsigned int tb = 0b001100010100011001110001100001;
//                    ....|....|....|....|....|....|

int ya, xa, yb, xb, yc, xc;

int sh0, sh1, sh2, sh3, sh4, sh5;

// constatnt shift add
int CSA = 1;

// initial constant multiplier
#define ICM 10

// shift value bit width
#define SVBW 5

void reinit() {
    ya=0;               xa=0737777<<ICM;
    yb=060000<<ICM;     xb=0;
    yc=0;               xc=020000<<ICM;

    std::bitset<30> t(tb);
    std::cout << t << std::endl;

    auto mask = ~(~unsigned(0) << SVBW);
    sh0 = ((tb >> SVBW*5) & mask) + CSA;
    sh1 = ((tb >> SVBW*4) & mask) + CSA;
    sh2 = ((tb >> SVBW*3) & mask) + CSA;
    sh3 = ((tb >> SVBW*2) & mask) + CSA;
    sh4 = ((tb >> SVBW*1) & mask) + CSA;
    sh5 = ((tb >> SVBW*0) & mask) + CSA;
    osc.clear();
}


void drawdot(uint32_t *p, int x, int y, double o, uint32_t c) {
    // keep 10 bits to wrap around 1024 screen pixels
#define SB (32-10)
    x = (x>>SB) + TEX_W/2;
    y = (y>>SB) + TEX_H/2;

    p[ y*TEX_W + x ] = c | ((unsigned)(0xff*o)<<24);
}

void update_pixels(uint32_t *p, int w, int h) {
    memset(p, 0, w*h*4);

    for (int i = 0; i<maxdots_perframe; ++i) {
        ya += (xa + xb) >> sh0;
        xa -= (ya - yb) >> sh1;

        yb += (xb - xc) >> sh2;
        xb -= (yb - yc) >> sh3;

        yc += (xc - xa) >> sh4;
        xc -= (yc - ya) >> sh5;

        osc.emplace_back(odot{xa, ya, xb, yb, xc, yc});
    }

    if (osc.size() > maxodots)
        osc.erase(osc.begin(), osc.end() - maxodots);

    const double N = osc.size();
    double i = 0;

    for (auto & oi : osc) {
        double o = (1-std::exp(-i*gm/N))/(1-std::exp(-gm));
        if (dots_clamped > 0) o /= 2;
        if (i > N - dots_clamped) o = 1;
        if (o<0) o=0;

        drawdot(p, oi.ax, oi.ay, o, ImGui::GetColorU32(ocolor1));
        drawdot(p, oi.bx, oi.by, o, ImGui::GetColorU32(ocolor2));
        drawdot(p, oi.cx, oi.cy, o, ImGui::GetColorU32(ocolor3));
        ++i;
    }
}



static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
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

    GLFWwindow* window = glfwCreateWindow(TEX_W, TEX_H, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync?

    const int    DATA_SIZE       = TEX_W * TEX_H * 4; // 4 channels

    GLuint image_texture;
    GLuint pboIds[2];
    int pbo_index = 0;

    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(2, pboIds);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImVec4 clear_color = ImVec4(0, 0, 0, 1.00f);

    reinit();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


        ImGui::Begin("32 bit clone of PDP-1 Minskytron");

        ScrollableSliderInt("Max dots", &maxodots, 1024, 1024*16, "%d", 256);
        ScrollableSliderInt("Dots per frame", &maxdots_perframe, 0, 4096, "%d", 8);
        ScrollableSliderInt("Dots clamped gamma", &dots_clamped, 0, maxodots, "%d", 8);
        ScrollableSliderFloat("Gamma", &gm, -8, 8, "%.2f", 0.2);

        if (BitField("Test word", &tb, 0))
            reinit();

        ImGui::ColorEdit3("clear color", (float*)&clear_color);

        ImGui::ColorEdit3("osc1 color", (float*)&ocolor1);
        ImGui::ColorEdit3("osc2 color", (float*)&ocolor2);
        ImGui::ColorEdit3("osc3 color", (float*)&ocolor3);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::End();



        int nexti = pbo_index;
        pbo_index = pbo_index ? 0 : 1;
        glBindTexture(GL_TEXTURE_2D, image_texture);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[pbo_index]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEX_W, TEX_H, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nexti]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
        uint32_t* ptr = (uint32_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        assert(ptr);
        update_pixels(ptr, TEX_W, TEX_H);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)image_texture,
            ImVec2(0, 0), ImVec2(display_w, display_h),
            ImVec2(0, 0), ImVec2((float)display_w/TEX_W, (float)display_h/TEX_H));


        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glDeleteTextures(1, &image_texture);
    glDeleteBuffers(2, pboIds);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
