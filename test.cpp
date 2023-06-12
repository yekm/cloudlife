#include <cstdint>
#include <memory>

#include <unistd.h> // getopt
#include <error.h>

#include "imgui.h"
#include "imgui_elements.h"

#include <stdio.h>

#include "timer.h"

#include "artfactory.h"

std::unique_ptr<Art> art;

static int sw = 1024, sh = 1024;

typedef uint32_t pixel_t;
pixel_t *image_data = NULL;
std::vector<pixel_t> image_data_vector;
#define texture_size (art->tex_w * art->tex_h * sizeof(pixel_t))

void make_pbos() {
    image_data_vector.resize(texture_size);
    image_data = image_data_vector.data();
}


int main(int argc, char *argv[])
{
    int opt;
    int frames = 128, iterations = 2, reporting = 1;

    while ((opt = getopt(argc, argv, "f:i:r:")) != -1) {
        switch (opt) {
        case 'f':
            frames = atoi(optarg);
            break;
        case 'i':
            iterations = atoi(optarg);
            break;
        case 'r':
            reporting = atoi(optarg);
            break;
        default: /* '?' */
            fprintf(stderr, "Usage: %s [-f frames=128] [-i iterations=2] [-r reporting=1]\n",
                    argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ArtFactory af;
    art = af.get_art();

    std::string first_art(art->name());

    do {
        sw = sh = 1024;
        for (int i=0; i<iterations; i++)
        {
            printf("%s %dx%d\n", art->name(), sw, sh);
            art->resized(sw, sh);
            art->frame_number = 0;
            make_pbos();

            float fps = 0;
            common::Timer old_t;

            while (art->frame_number < frames)
            {
                art->draw(image_data);

                ++art->frame_number;
                if (art->clear_every != 0 && art->frame_number % art->clear_every == 0)
                    art->clear();

                if (art->frame_number % reporting == 0) {
                    common::Timer t;
                    double dt = (t - old_t).seconds();
                    double fps = art->frame_number / dt;

                    printf("%s frame:%d fps:%.2f                \r",
                        cpu_load_text(), art->frame_number, fps);
                    fflush(stdout);
                }
            }
            sw += 321;
            sh += 123;
            printf("\n");
        }

        af.cycle_art();
        art = af.get_art();
    } while (first_art != art->name());

    ImGui::DestroyContext();

    return 0;
}
