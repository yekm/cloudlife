#pragma once
#include "art.hpp"

#include <string>

class SphereOfCubes : public Art {
public:
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
