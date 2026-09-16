#pragma once

// https://www.shadertoy.com/view/7ds3zB

#include "art.hpp"

#include <string>

class SphereOfCubes : public Art {
public:
    std::string about() const override
    {
        return "History and provenance\n"
               "The header points to Shadertoy shader 7ds3zB as the reference for this art. The available local "
               "comments do not name its original author or publication date, or the author and date of the "
               "Cloudlife adaptation. The reference is retained below without inventing those credits. This "
               "implementation turns the effect into an OpenGL compute shader with interactive camera and "
               "geometry parameters.\n"
               "\n"
               "Algorithm\n"
               "A perspective camera orbits the origin in the horizontal plane at the configured height. Its "
               "view direction points toward the origin; cross products construct horizontal and vertical image "
               "axes. Aperture determines the projection distance, and each pixel launches a normalized ray "
               "through that camera's image plane.\n"
               "\n"
               "An analytic ray-sphere intersection first solves a quadratic to find entry and exit distances. "
               "Rays that miss retain a dark background. A regular cube lattice approximates the sphere: cube "
               "centers are considered solid when they lie inside a reduced radius, equal to sphere radius minus "
               "half a cube's space diagonal. This reduction keeps selected cubes within the outer spherical "
               "envelope. The entry point is rounded to a lattice center. Grid traversal advances to the nearest "
               "next cell boundary along x, y, or z, stopping when it reaches a solid neighboring cell or leaves "
               "the outer sphere. The loop is capped at 51 boundary steps, so this is a bounded traversal rather "
               "than an unlimited geometric search.\n"
               "\n"
               "The crossed axis determines face color: red, green, or blue. Nearby side and diagonal cells are "
               "checked for solidity; nonlinear smoothstep-like functions darken face edges and corners beside "
               "those neighbors. This local shading resembles ambient occlusion but does not launch secondary "
               "shadow rays. The shader writes that result directly, using no polygon mesh or palette lookup.\n"
               "\n"
               "Cube width changes voxel size; sphere radius sets the envelope. Camera distance and height "
               "change the view, rotation speed sets orbital animation and direction, and aperture changes the "
               "field of view.\n"
               "\n"
               "References\n"
               "https://www.shadertoy.com/view/7ds3zB\n"
               "https://en.wikipedia.org/wiki/Ray_tracing_(graphics)\n"
               "https://en.wikipedia.org/wiki/Voxel\n"
               "https://en.wikipedia.org/wiki/Ambient_occlusion";
    }

    SphereOfCubes();

private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;
    
    void init_shader();
    void update_uniform_callback();

    // Shader parameters
    float cube_width = 0.2f;
    float sphere_radius = 1.0f;
    float camera_distance = 2.0f;
    float camera_height = 1.5f;
    float rotation_speed = 0.2f;
    float aperture = 50.0f;

    std::string current_colormap_name;
    
    // Compute shader base source (without colormap)
    static const char* sphere_compute_shader_base;
};
