#include "plasmacompute.h"
#include "easelcompute.h"

#include "imgui_elements.h"

// Compute shader base for plasma effect (colormap will be appended)
const char* PlasmaCompute::plasma_compute_shader_base = R"(
    #version 430 core
    
    layout(local_size_x = 16, local_size_y = 16) in;
    
    layout(rgba8, binding = 0) uniform writeonly image2D output_image;
    
    uniform vec2 u_resolution;
    uniform float u_time;
    uniform float u_speed;
    uniform float u_scale;
    
    // Colormap function will be injected here
    vec4 colormap(float x);
    
    // Simplex noise functions
    vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec2 mod289(vec2 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
    vec3 permute(vec3 x) { return mod289(((x*34.0)+1.0)*x); }
    
    float snoise(vec2 v) {
        const vec4 C = vec4(0.211324865405187, 0.366025403784439,
                           -0.577350269189626, 0.024390243902439);
        vec2 i  = floor(v + dot(v, C.yy));
        vec2 x0 = v -   i + dot(i, C.xx);
        vec2 i1;
        i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
        vec4 x12 = x0.xyxy + C.xxzz;
        x12.xy -= i1;
        i = mod289(i);
        vec3 p = permute( permute( i.y + vec3(0.0, i1.y, 1.0 ))
            + i.x + vec3(0.0, i1.x, 1.0 ));
        vec3 m = max(0.5 - vec3(dot(x0,x0), dot(x12.xy,x12.xy), dot(x12.zw,x12.zw)), 0.0);
        m = m*m;
        m = m*m;
        vec3 x = 2.0 * fract(p * C.www) - 1.0;
        vec3 h = abs(x) - 0.5;
        vec3 ox = floor(x + 0.5);
        vec3 a0 = x - ox;
        m *= 1.79284291400159 - 0.85373472095314 * ( a0*a0 + h*h );
        vec3 g;
        g.x  = a0.x  * x0.x  + h.x  * x0.y;
        g.yz = a0.yz * x12.xz + h.yz * x12.yw;
        return 130.0 * dot(m, g);
    }
    
    void main() {
        ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
        
        if (pixel_coords.x >= int(u_resolution.x) || pixel_coords.y >= int(u_resolution.y))
            return;
        
        vec2 uv = vec2(pixel_coords) / u_resolution;
        float t = u_time * u_speed;
        
        // Plasma effect using multiple sine waves
        float v1 = sin(uv.x * u_scale * 10.0 + t);
        float v2 = sin(uv.y * u_scale * 10.0 + t * 0.7);
        float v3 = sin((uv.x + uv.y) * u_scale * 5.0 + t * 1.3);
        float v4 = sin(sqrt(uv.x*uv.x + uv.y*uv.y) * u_scale * 8.0 + t * 0.5);
        
        // Add some noise
        float n = snoise(uv * u_scale * 3.0 + t * 0.1) * 0.3;
        
        float plasma = (v1 + v2 + v3 + v4 + n) / 5.0;
        plasma = plasma * 0.5 + 0.5; // Normalize to 0-1
        
        // Use colormap for coloring
        vec4 color = colormap(plasma);
        
        imageStore(output_image, pixel_coords, color);
    }
)";

PlasmaCompute::PlasmaCompute()
    : Art("Plasma (Compute Shader)") {
    useCompute();
    current_colormap_name = easel->pal.get_cmap().getTitle();
    init_shader();
}

void PlasmaCompute::init_shader() {
    // Combine base shader with colormap source
    std::string full_shader = plasma_compute_shader_base + easel->pal.get_cmap().getSource();
    ecompute()->set_compute_shader(full_shader);

    // Set initial uniform values
    ecompute()->set_uniform_float("u_speed", speed);
    ecompute()->set_uniform_float("u_scale", scale);
}

bool PlasmaCompute::render(uint32_t *p) {
    return true;
}

bool PlasmaCompute::render_gui() {
    bool up = false;
    
    // Check if colormap changed (palette GUI rendered by EaselCompute::gui())
    std::string new_colormap = easel->pal.get_cmap().getTitle();
    if (new_colormap != current_colormap_name) {
        current_colormap_name = new_colormap;
        init_shader();
        up = true;
    }
    
    // Update uniforms immediately when GUI values change
    up |= ScrollableSliderFloat("Speed", &speed, 0.0f, 5.0f, "%.2f", 0.1f);
    up |= ScrollableSliderFloat("Scale", &scale, 0.5f, 10.0f, "%.2f", 0.1f);
    
    if (up) {
        // Apply uniform updates immediately when GUI changes
        ecompute()->set_uniform_float("u_speed", speed);
        ecompute()->set_uniform_float("u_scale", scale);
    }

    return false;
}

void PlasmaCompute::resize(int _w, int _h) {
    default_resize(_w, _h);
}
