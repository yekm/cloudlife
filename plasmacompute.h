#pragma once
#include "art.hpp"

#include <string>

class PlasmaCompute : public Art {
public:
    PlasmaCompute();

private:
    virtual bool render_gui() override;
    virtual void resize(int _w, int _h) override;
    virtual bool render(uint32_t *p) override;
    
    void init_shader();
    void update_uniform_callback();

    // Shader parameters
    float speed = 1.0f;
    float scale = 3.0f;

    std::string current_colormap_name;
    
    // Compute shader base source (without colormap)
    static const char* plasma_compute_shader_base;
};
