#include "artfactory.h"

#include "imgui.h"

#include <glad/glad.h>

#include <algorithm>
#include <cstdio>

#include "cloudlife.hpp"
#include "mtron.hpp"
#include "ifs.h"
#include "vermiculate.h"
#include "discrete.h"
#include "thornbird.h"
#include "onepixel.h"
#include "rdbomb.h"
#include "acidwarp.h"
#include "acidworm.h"
#include "hopalong.h"
#include "hopalong3d.hpp"
#include "collatzbirb3d.hpp"
#include "primeumap3d.hpp"
#include "attractor.h"
#include "test3d.hpp"
#include "physarum.hpp"
#ifndef __APPLE__
#include "acidwarpgpt56terra.h"
#include "acidwarpgpt56luna.h"
#include "acidwarpqwen38max.h"
#include "acidwarpm3.h"
#include "plasmacompute.h"
#include "sphereofcubes.h"
#include "physarumgpu.hpp"
#endif

namespace {

constexpr unsigned DEFAULT_ART_ID = 1;
constexpr const char* OPENGL_33_REQUIREMENT = "requires OpenGL 3.3 / GLSL 3.30";
constexpr const char* OPENGL_43_COMPUTE_REQUIREMENT =
    "requires OpenGL 4.3 compute shaders / GLSL 4.30";

} // namespace


ArtFactory::ArtFactory() {
    const bool has_opengl_33 = GLAD_GL_VERSION_3_3 != 0;
#ifndef __APPLE__
    const bool has_opengl_43_compute = GLAD_GL_VERSION_4_3 != 0;
#endif

    add_art<Test3D>("Test3D", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<Cloudlife>("Cloudlife");
    add_art<Minskytron>("Minskytron");
    add_art<IFS>("IFS");
    add_art<Vermiculate>("Vermiculate");
    add_art<Discrete>("Discrete", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<Thornbird>("Thornbird");
    add_art<OnePixel>("OnePixel", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<Nothing>("Nothing", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<RDbomb>("RDbomb");
    add_art<AcidWarp>("AcidWarp");
    add_art<AcidWorm>("AcidWorm");
    add_art<Hopalong>("Hopalong", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<Hopalong3D>("Hopalong 3D", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<CollatzBirb3D>("Collatz Birb 3D", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<Attractor>("Attractor", has_opengl_33, OPENGL_33_REQUIREMENT);
    add_art<Physarum>("Physarum");
#ifndef __APPLE__
    add_art<AcidWarpGpt56Terra>("AcidWarp-gpt56-terra", has_opengl_43_compute,
                               OPENGL_43_COMPUTE_REQUIREMENT);
    add_art<AcidWarpGpt56Luna>("AcidWarp-gpt56-luna", has_opengl_43_compute,
                              OPENGL_43_COMPUTE_REQUIREMENT);
    add_art<AcidWarpQwen38Max>("AcidWarp-qwen38-max", has_opengl_43_compute,
                              OPENGL_43_COMPUTE_REQUIREMENT);
    add_art<AcidWarpM3>("AcidWarp-minimaxm3", has_opengl_43_compute,
                       OPENGL_43_COMPUTE_REQUIREMENT);
    add_art<PlasmaCompute>("Plasma (Compute)", has_opengl_43_compute,
                           OPENGL_43_COMPUTE_REQUIREMENT);
    add_art<SphereOfCubes>("Sphere of Cubes", has_opengl_43_compute,
                          OPENGL_43_COMPUTE_REQUIREMENT);
    add_art<PhysarumGPU>("PhysarumGPU", has_opengl_43_compute,
                        OPENGL_43_COMPUTE_REQUIREMENT);
#else
    add_unavailable_art("AcidWarp-gpt56-terra", OPENGL_43_COMPUTE_REQUIREMENT);
    add_unavailable_art("AcidWarp-gpt56-luna", OPENGL_43_COMPUTE_REQUIREMENT);
    add_unavailable_art("AcidWarp-qwen38-max", OPENGL_43_COMPUTE_REQUIREMENT);
    add_unavailable_art("AcidWarp-minimaxm3", OPENGL_43_COMPUTE_REQUIREMENT);
    add_unavailable_art("Plasma (Compute)", OPENGL_43_COMPUTE_REQUIREMENT);
    add_unavailable_art("Sphere of Cubes", OPENGL_43_COMPUTE_REQUIREMENT);
    add_unavailable_art("PhysarumGPU", OPENGL_43_COMPUTE_REQUIREMENT);
#endif
    add_art<PrimeUmap3D>("Prime UMAP 3D", has_opengl_33, OPENGL_33_REQUIREMENT);

    vc = VectorCombo("Art", art_items);
    if (!select_available_art(DEFAULT_ART_ID) && !available_art_ids.empty())
        select_available_art(available_art_ids.front());

    report_unavailable_arts();
}

std::unique_ptr<Art> ArtFactory::get_art() {
    return std::unique_ptr<Art>(create(vc.get_value()));
}

void ArtFactory::cycle_art() {
    if (art_items.empty())
        return;

    int i = vc.get_index() + 1;
    if (i >= static_cast<int>(art_items.size()))
        i = 0;
    vc.set_index(i);
}

void ArtFactory::set_art(unsigned a) {
    if (a >= art_catalogue.size()) {
        fprintf(stderr, "Art ID %u does not exist; selecting default art %u instead\n",
                a, fallback_art_id());
    } else if (!art_catalogue[a].available) {
        fprintf(stderr, "Art %u (%s) is unavailable: %s; selecting default art %u instead\n",
                a, art_catalogue[a].name.c_str(), art_catalogue[a].unavailable_reason.c_str(),
                fallback_art_id());
    } else {
        select_available_art(a);
        return;
    }

    select_available_art(fallback_art_id());
}

bool ArtFactory::render_gui() {
    const bool changed = vc.RenderGui();

    int unavailable_count = 0;
    for (const auto& entry : art_catalogue) {
        if (!entry.available)
            ++unavailable_count;
    }

    if (unavailable_count > 0 &&
        ImGui::TreeNode("UnavailableArts", "Unavailable arts (%d)", unavailable_count)) {
        for (unsigned id = 0; id < art_catalogue.size(); ++id) {
            const auto& entry = art_catalogue[id];
            if (!entry.available)
                ImGui::BulletText("%u: %s (%s)", id, entry.name.c_str(),
                                  entry.unavailable_reason.c_str());
        }
        ImGui::TreePop();
    }

    return changed;
}

void ArtFactory::add_unavailable_art(const std::string& name,
                                     const std::string& unavailable_reason) {
    art_catalogue.push_back({name, false, unavailable_reason});
}

bool ArtFactory::select_available_art(unsigned catalogue_id) {
    const auto selected = std::find(available_art_ids.begin(), available_art_ids.end(), catalogue_id);
    if (selected == available_art_ids.end())
        return false;

    vc.set_index(static_cast<int>(std::distance(available_art_ids.begin(), selected)));
    return true;
}

unsigned ArtFactory::fallback_art_id() const {
    if (DEFAULT_ART_ID < art_catalogue.size() && art_catalogue[DEFAULT_ART_ID].available)
        return DEFAULT_ART_ID;
    if (!available_art_ids.empty())
        return available_art_ids.front();
    return 0;
}

void ArtFactory::report_unavailable_arts() const {
    const auto* gl_version = glad_glGetString
        ? reinterpret_cast<const char*>(glGetString(GL_VERSION))
        : nullptr;
    for (unsigned id = 0; id < art_catalogue.size(); ++id) {
        const auto& entry = art_catalogue[id];
        if (entry.available)
            continue;

        fprintf(stderr, "Art %u (%s) unavailable: %s (runtime: %s)\n", id,
                entry.name.c_str(), entry.unavailable_reason.c_str(),
                gl_version ? gl_version : "OpenGL version unknown");
    }
}
