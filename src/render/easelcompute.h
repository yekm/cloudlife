#pragma once

#ifndef __APPLE__

#include "easel.h"

#define GL_GLEXT_PROTOTYPES 1
#define GL3_PROTOTYPES 1
#include <GLFW/glfw3.h>

#include <string>
#include <functional>
#include <unordered_map>


class EaselCompute : public Easel {
public:
    // Callback to let Art class set uniforms before dispatch
    using UniformCallback = std::function<void(GLuint program)>;

    EaselCompute();
    ~EaselCompute();

    // Required by base class - no-op for compute (pixels generated on GPU)
    void drawdot(int32_t x, int32_t y, uint32_t c) override;

    // Set compute shader source
    void set_compute_shader(const std::string& source);
    
    // Set callback for uniform updates
    void set_uniform_callback(UniformCallback callback);
    
    // Dispatch compute (called automatically in render())
    void dispatch();

    virtual void begin() override;
    virtual void render() override;
    virtual void clear() override;
    virtual void gui() override;
    virtual void reset() override;

    // Uniform helpers
    void set_uniform_float(const std::string& name, float value);
    void set_uniform_int(const std::string& name, int value);
    void set_uniform_vec2(const std::string& name, float x, float y);
    void set_uniform_vec3(const std::string& name, float x, float y, float z);
    void set_uniform_vec4(const std::string& name, float x, float y, float z, float w);

private:
    void create_texture();
    void destroy_texture();
    void compile_shader();
    void cleanup_shader();
    int get_uniform_location(const std::string& name);

    GLuint compute_program = 0;
    GLuint output_texture = 0;
    GLuint pbo = 0;  // Optional: for reading back to CPU if needed
    
    std::string compute_source;
    UniformCallback uniform_callback;
    
    // Cached uniform locations
    std::unordered_map<std::string, GLint> uniform_locations;
    
    // Built-in uniforms
    GLint u_resolution_loc = -1;
    GLint u_time_loc = -1;
    GLint u_frame_loc = -1;
    
    // Stats
    unsigned dispatch_count = 0;
    double total_dispatch_time_ms = 0;

    // Work group size for compute dispatch (queried from device)
    int work_group_size[3] = {16, 16, 1};  // Default fallback
    void query_work_group_size();

    // Default compute shader - simple gradient
    static const char* default_compute_shader;
};

#else // __APPLE__

// Empty stub on macOS - compute shaders not supported
class EaselCompute {};

#endif // __APPLE__
