#include "artfactory.h"

#include <glad/glad.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

void expect_art(ArtFactory& factory, unsigned id, const char* name)
{
    factory.set_art(id);
    const auto art = factory.get_art();
    require(art && std::strcmp(art->name(), name) == 0, "catalogue ID selected the wrong art");
}
}

int main()
{
    // Simulate the minimum context with no GL function pointers. Any attempted
    // construction of a vertex or compute renderer would fail immediately.
    GLAD_GL_VERSION_3_3 = 0;
    GLAD_GL_VERSION_4_3 = 0;
    ArtFactory factory;
    const char* default_name = "Cloudlife from xscreensaver";
    for (unsigned id : {0u, 5u, 7u, 8u, 12u, 13u, 14u, 15u, 17u, 18u, 19u, 20u, 21u, 22u, 23u, 99u})
        expect_art(factory, id, default_name);

    for (const char* name : {"Test3D", "Discrete", "OnePixel", "Nothing", "Hopalong",
                            "Hopalong 3D", "Collatz Birb 3D", "Attractor", "AcidWarp-gpt56-terra",
                            "AcidWarp-gpt56-luna", "AcidWarp-qwen38-max", "AcidWarp-minimaxm3",
                            "Plasma (Compute)", "Sphere of Cubes", "PhysarumGPU"}) {
        require(factory.create(name) == nullptr, "unsupported art constructor was registered");
    }

    // Removing earlier entries from the selector must not renumber CLI IDs.
    expect_art(factory, 10, "AcidWarp");
    expect_art(factory, 11, "AcidWorm");
    expect_art(factory, 16, "Physarum");
    factory.cycle_art();
    require(std::strcmp(factory.get_art()->name(), "Marbling") == 0,
            "cycling must skip unsupported entries to reach Marbling");
    factory.set_art(4);
    factory.cycle_art();
    require(std::strcmp(factory.get_art()->name(), "thornbird --- continuously varying Thornbird set") == 0,
            "cycling must skip unsupported vertex entries");

    std::puts("Art capability filtering, stable IDs, fallback and cycling passed.");
}
