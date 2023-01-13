#include "colormap/include/colormap/color.hpp"
#include <cstdint>
#include <deque>
#include <bitset>
#include <iostream>

#include <colormap/colormap.hpp>

#include "random.h"

#include <math.h> // exp
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

//////////////////////////////////////////////////////////////////////////////

static int item_current_idx = 0;

auto pal = colormap::palettes.at("inferno");

bool draw_pal_combo() {
    bool ret = false;
    auto pb = colormap::palettes.begin();
    std::advance(pb, item_current_idx);

    int size = colormap::palettes.size();
    const char* combo_preview_value = pb->first.c_str();
    if (ImGui::BeginCombo("Palette", combo_preview_value, 0))
    {
        auto pb = colormap::palettes.begin();
        for (int n = 0; n < size; n++)
        {
            const bool is_selected = (item_current_idx == n);
            const char * name = pb->first.c_str();
            if (ImGui::Selectable(name, is_selected))
            {
                item_current_idx = n;
                ret = true;
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
            std::advance(pb, 1);
        }
        ImGui::EndCombo();
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////
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
static int sw = 1024, sh = 1024;

#define DATA_SIZE  (sw * sh * 4)


bool get_size(int *w, int *h) {
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

void make_pbos() {

    image_data = (uint32_t*)xrealloc(image_data, DATA_SIZE);

    // AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
    // http://www.songho.ca/opengl/gl_pbo.html
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sw, sh, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenBuffers(2, pboIds);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[0]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[1]);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void destroy_pbos() {
    glDeleteTextures(1, &image_texture);
    glDeleteBuffers(2, pboIds);
}

// -----------------------------------------------------------------------------

void drawdot(uint32_t *p, int x, int y, double o, uint32_t c) {
    p[ y*sw + x ] = c | ((unsigned)(0xff*o)<<24);
}

void drawdot(uint32_t *p, int x, int y, uint32_t c) {
    p[ y*sw + x ] = c;
}


//////////////////////////////////////////////

static struct field {
    unsigned int height;
    unsigned int width;
    unsigned int max_age;
    unsigned int cell_size;
    unsigned char *cells;
    unsigned char *new_cells;
    unsigned int ticks_per_frame;
    uint32_t *image;
} field_;

struct field *f = &field_;

ImVec4 clear_color = ImVec4(1, 0, 0, 1.00f);
ImVec4 background = ImVec4(0, 0, 0, 1);
ImVec4 foreground = ImVec4(0, 1, 0, 1);

int density = 32, cycles=0;

////////////////////////////////////////////////

static void
resize_field(unsigned int w, unsigned int h)
{
    int s = w * h * sizeof(unsigned char);
    f->width = w;
    f->height = h;

    f->cells = (unsigned char*)xrealloc(f->cells, s);
    f->new_cells = (unsigned char*)xrealloc(f->new_cells, s);
    f->image = (uint32_t*)xrealloc(f->image, DATA_SIZE);
    memset(f->cells, 0, s);
    memset(f->new_cells, 0, s);
    auto pb = colormap::palettes.begin();
    std::advance(pb, item_current_idx);
    pal = pb->second.rescale(0., f->max_age);
    //pal = colormap::palettes.at("plasma").rescale(0., f->max_age);
}

void init_field()
{
    f->height = 0;
    f->width = 0;
    f->cell_size = 3;
    f->max_age = 64;
    f->ticks_per_frame = 1;

    f->cells = NULL;
    f->new_cells = NULL;
    f->image = NULL;
}




static unsigned int
random_cell(unsigned int p)
{
    int r = xoshiro256plus() & 0xff;

    if (r < p) {
        return (1);
    } else {
        return (0);
    }
}

static inline unsigned char
*cell_at(unsigned int x, unsigned int y)
{
    return (f->cells + x * sizeof(unsigned char) +
                       y * f->width * sizeof(unsigned char));
}

static inline unsigned char
*new_cell_at(unsigned int x, unsigned int y)
{
    return (f->new_cells + x * sizeof(unsigned char) +
                           y * f->width * sizeof(unsigned char));
}

static void
populate_field(unsigned int p)
{
    unsigned int x, y;

    for (x = 0; x < f->width; x++) {
        for (y = 0; y < f->height; y++) {
            *cell_at(x, y) = random_cell(p);
        }
    }
}

static void
refield() {
    resize_field(sw / (1 << f->cell_size) + 2,
                sh / (1 << f->cell_size) + 2);
    populate_field(density);
    memset(f->image, 0, DATA_SIZE);
}

// --------------------------------------------------

static void draw_gui() {
    bool up = false;

    up |= ScrollableSliderInt("Initial density", &density, 8, 256, "%d", 8);
    up |= ScrollableSliderUInt("Cell size", &f->cell_size, 1, 64, "%d", 1);
    up |= ScrollableSliderUInt("Max age", &f->max_age, 4, 256, "%d", 8);
    ScrollableSliderUInt("Ticks per frame", &f->ticks_per_frame, 1, 128, "%d", 1);
    up |= ImGui::ColorEdit4("Foreground", (float*)&foreground);
    up |= ImGui::ColorEdit4("Backgroud", (float*)&background);
    up |= ImGui::ColorEdit4("Clear color", (float*)&clear_color);
    up |= draw_pal_combo();

    if (up) {
        refield();
    }
}


static void
populate_edges(unsigned int p)
{
    unsigned int i;

    for (i = f->width; i--;) {
        *cell_at(i, 0) = random_cell(p);
        *cell_at(i, f->height - 1) = random_cell(p);
    }

    for (i = f->height; i--;) {
        *cell_at(f->width - 1, i) = random_cell(p);
        *cell_at(0, i) = random_cell(p);
    }
}

//--------------------------------------------------------------

uint32_t get_color_age(colormap::map<colormap::color<colormap::space::rgb>> &m, uint8_t age) {
    auto c = m(age);
    return  0xff000000 |
            c.getRed().getValue() << 0 |
            c.getGreen().getValue() << 8 |
            c.getBlue().getValue() << 16;
}

static void
draw_field(uint32_t *p)
{
    unsigned int x, y;
    unsigned int rx, ry = 0;	/* random amount to offset the dot */
    unsigned int size = 1 << f->cell_size;
    unsigned int mask = size - 1;
    unsigned int fg_count, bg_count;
    uint32_t fgc, bgc;

    fgc = ImGui::GetColorU32(foreground);
    bgc = ImGui::GetColorU32(background);

    /* columns 0 and width-1 are off screen and not drawn. */
    for (y = 1; y < f->height - 1; y++) {
        fg_count = 0;
        bg_count = 0;

        /* rows 0 and height-1 are off screen and not drawn. */
        for (x = 1; x < f->width - 1; x++) {
            rx = xoshiro256plus();
            ry = rx >> f->cell_size;
            rx &= mask;
            ry &= mask;

            uint8_t age = *cell_at(x, y);
            if (age) {
                drawdot(p,
                        (short) x *size - rx - 1,
                        (short) y *size - ry - 1,
                        get_color_age(pal, age));
            } else {
                drawdot(p, (short) x *size - rx - 1,
                        (short) y *size - ry - 1,
                        bgc);
            }
        }
    }
}

static inline unsigned int
cell_value(unsigned char c, unsigned int age)
{
    if (!c) {
        return 0;
    } else if (c > age) {
        return (3);
    } else {
        return (1);
    }
}

static inline unsigned int
is_alive(unsigned int x, unsigned int y)
{
    unsigned int count;
    unsigned int i, j;
    unsigned char *p;

    count = 0;

    for (i = x - 1; i <= x + 1; i++) {
        for (j = y - 1; j <= y + 1; j++) {
            if (y != j || x != i) {
                count += cell_value(*cell_at(i, j), f->max_age);
            }
        }
    }

    p = cell_at(x, y);
    if (*p) {
        if (count == 2 || count == 3) {
            return ((*p) + 1);
        } else {
            return (0);
        }
    } else {
        if (count == 3) {
            return (1);
        } else {
            return (0);
        }
    }
}

static unsigned int
do_tick()
{
    unsigned int x, y;
    unsigned int count = 0;
    for (x = 1; x < f->width - 1; x++) {
        for (y = 1; y < f->height - 1; y++) {
            count += *new_cell_at(x, y) = is_alive(x, y);
        }
    }
    memcpy(f->cells, f->new_cells, f->width * f->height *
           sizeof(unsigned char));
    return count;
}

static void
draw_life(uint32_t *p) {
    unsigned int count = 0;

    for (int i=0; i<f->ticks_per_frame; ++i)
        count = do_tick();

    if (count < (f->height + f->width) / 4) {
        populate_field(density);
    }

    if (cycles % (f->max_age / 2) == 0) {
        populate_edges(density);
        do_tick();
        populate_edges(0);
    }

    draw_field(f->image);

    memcpy(p, f->image, DATA_SIZE);

    cycles++;
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

    make_pbos();
    init_field();
    refield();


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();



        ImGui::Begin("Cloudlife from xscreensaver");

        draw_gui();

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                    1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);

        ImGui::End();

        if (get_size(0,0)) {
            destroy_pbos();
            make_pbos();
            refield();
        }

        int nexti = pbo_index;
        pbo_index = pbo_index ? 0 : 1;
        glBindTexture(GL_TEXTURE_2D, image_texture);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[pbo_index]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboIds[nexti]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, DATA_SIZE, 0, GL_STREAM_DRAW);
        uint32_t* ptr = (uint32_t*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        assert(ptr);
        draw_life(ptr);
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        ImGui::GetBackgroundDrawList()->AddImage((void*)(intptr_t)image_texture,
            ImVec2(0, 0), ImVec2(sw, sh));


        ImGui::Render();
        glViewport(0, 0, sw, sh);
        //glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        //glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    destroy_pbos();
    free(image_data);
    free(f->cells);
    free(f->new_cells);
    free(f->image);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
