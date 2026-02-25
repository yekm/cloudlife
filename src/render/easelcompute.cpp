#include "easelcompute.h"

#include <stdio.h>
#include <iostream>
#include <chrono>

#include "imgui.h"

// Default compute shader - generates a simple animated gradient
const char* EaselCompute::default_compute_shader = R"(
    #version 430 core
    
    layout(local_size_x = 16, local_size_y = 16) in;
    
    layout(rgba8, binding = 0) uniform writeonly image2D output_image;
    
    uniform vec2 u_resolution;
    uniform float u_time;
    uniform int u_frame;
    
    void main() {
        ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
        
        // Check bounds
        if (pixel_coords.x >= int(u_resolution.x) || pixel_coords.y >= int(u_resolution.y))
            return;
        
        // Normalized coordinates
        vec2 uv = vec2(pixel_coords) / u_resolution;
        
        // Simple animated gradient
        vec3 color = vec3(
            0.5 + 0.5 * sin(uv.x * 3.14159 * 2.0 + u_time),
            0.5 + 0.5 * sin(uv.y * 3.14159 * 2.0 + u_time * 0.7),
            0.5 + 0.5 * sin((uv.x + uv.y) * 3.14159 + u_time * 1.3)
        );
        
        imageStore(output_image, pixel_coords, vec4(color, 1.0));
    }
)";

void EaselCompute::query_work_group_size() {
    // Query device-specific work group limits for optimal performance
    GLint max_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_size[1]);

    // Use a reasonable default (256 total threads per work group is widely supported)
    // 16x16 = 256 threads
    work_group_size[0] = 16;
    work_group_size[1] = 16;

    // Clamp to device limits
    if (work_group_size[0] > max_size[0]) work_group_size[0] = max_size[0];
    if (work_group_size[1] > max_size[1]) work_group_size[1] = max_size[1];
}

EaselCompute::EaselCompute() {
    query_work_group_size();
    compute_source = default_compute_shader;
    compile_shader();
    create_texture();
}

EaselCompute::~EaselCompute() {
    cleanup_shader();
    destroy_texture();
}

void EaselCompute::create_texture() {
    if (w <= 0 || h <= 0) return;

    glGenTextures(1, &output_texture);
    glBindTexture(GL_TEXTURE_2D, output_texture);

    // Create immutable storage for compute shader writing
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, w, h);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Bind to image unit 0 for compute shader - this binding persists
    glBindImageTexture(0, output_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
}

void EaselCompute::destroy_texture() {
    if (output_texture) {
        glDeleteTextures(1, &output_texture);
        output_texture = 0;
    }
}

void EaselCompute::compile_shader() {
    GLuint compute_shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* source = compute_source.c_str();
    glShaderSource(compute_shader, 1, &source, NULL);
    glCompileShader(compute_shader);
    
    // Check compilation
    GLint success;
    glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetShaderInfoLog(compute_shader, 512, NULL, info_log);
        std::cerr << "ERROR::COMPUTE_SHADER::COMPILATION_FAILED\n" << info_log << std::endl;
        glDeleteShader(compute_shader);
        return;
    }
    
    compute_program = glCreateProgram();
    glAttachShader(compute_program, compute_shader);
    glLinkProgram(compute_program);
    
    // Check linking
    glGetProgramiv(compute_program, GL_LINK_STATUS, &success);
    if (!success) {
        char info_log[512];
        glGetProgramInfoLog(compute_program, 512, NULL, info_log);
        std::cerr << "ERROR::COMPUTE_PROGRAM::LINKING_FAILED\n" << info_log << std::endl;
    }
    
    glDeleteShader(compute_shader);
    
    // Cache built-in uniform locations
    u_resolution_loc = glGetUniformLocation(compute_program, "u_resolution");
    u_time_loc = glGetUniformLocation(compute_program, "u_time");
    u_frame_loc = glGetUniformLocation(compute_program, "u_frame");
    
    // Clear cached locations since program changed
    uniform_locations.clear();
}

void EaselCompute::cleanup_shader() {
    if (compute_program) {
        glDeleteProgram(compute_program);
        compute_program = 0;
    }
}

void EaselCompute::set_compute_shader(const std::string& source) {
    cleanup_shader();
    compute_source = source;
    compile_shader();
}

void EaselCompute::set_uniform_callback(UniformCallback callback) {
    uniform_callback = callback;
}

int EaselCompute::get_uniform_location(const std::string& name) {
    auto it = uniform_locations.find(name);
    if (it != uniform_locations.end()) {
        return it->second;
    }
    
    GLint loc = glGetUniformLocation(compute_program, name.c_str());
    uniform_locations[name] = loc;
    return loc;
}

void EaselCompute::set_uniform_float(const std::string& name, float value) {
    GLint loc = get_uniform_location(name);
    if (loc >= 0) glUniform1f(loc, value);
}

void EaselCompute::set_uniform_int(const std::string& name, int value) {
    GLint loc = get_uniform_location(name);
    if (loc >= 0) glUniform1i(loc, value);
}

void EaselCompute::set_uniform_vec2(const std::string& name, float x, float y) {
    GLint loc = get_uniform_location(name);
    if (loc >= 0) glUniform2f(loc, x, y);
}

void EaselCompute::set_uniform_vec3(const std::string& name, float x, float y, float z) {
    GLint loc = get_uniform_location(name);
    if (loc >= 0) glUniform3f(loc, x, y, z);
}

void EaselCompute::set_uniform_vec4(const std::string& name, float x, float y, float z, float w) {
    GLint loc = get_uniform_location(name);
    if (loc >= 0) glUniform4f(loc, x, y, z, w);
}

void EaselCompute::drawdot(int32_t x, int32_t y, uint32_t c) {
    // No-op for compute shader - pixels are generated on GPU
    // This method exists only to satisfy the base class interface
}

void EaselCompute::begin() {
    // Nothing to do before compute dispatch
}

void EaselCompute::dispatch() {
    if (!compute_program || !output_texture) return;

    auto start = std::chrono::high_resolution_clock::now();

    glUseProgram(compute_program);

    // Note: output image is bound to unit 0 in create_texture() and persists

    // Set built-in uniforms
    if (u_resolution_loc >= 0) {
        glUniform2f(u_resolution_loc, static_cast<float>(w), static_cast<float>(h));
    }
    if (u_time_loc >= 0) {
        float time = static_cast<float>(glfwGetTime());
        glUniform1f(u_time_loc, time);
    }
    if (u_frame_loc >= 0) {
        glUniform1i(u_frame_loc, static_cast<int>(dispatch_count));
    }
    
    // Call user callback for additional uniforms
    if (uniform_callback) {
        uniform_callback(compute_program);
    }
    
    // Dispatch compute shader with dynamic work group size
    GLuint groups_x = (w + work_group_size[0] - 1) / work_group_size[0];
    GLuint groups_y = (h + work_group_size[1] - 1) / work_group_size[1];
    glDispatchCompute(groups_x, groups_y, 1);
    
    // Memory barrier: ensure shader image writes complete before texture sampling
    // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT: sync imageStore operations
    // GL_TEXTURE_FETCH_BARRIER_BIT: sync texture sampling (used by ImGui)
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(end - start);
    total_dispatch_time_ms += duration.count();
    dispatch_count++;
}

void EaselCompute::render() {
    // Dispatch compute to generate the image
    dispatch();
    
    // Draw the generated texture as a fullscreen quad
    ImGui::GetBackgroundDrawList()->AddImage(
        (void*)(intptr_t)output_texture,
        ImVec2(0, 0), 
        ImVec2(ww, wh),
        ImVec2(0, 0), 
        ImVec2(1, 1)
    );
}

void EaselCompute::clear() {
    // Clear the texture using glTexSubImage2D for compatibility (OpenGL 3.0+)
    if (output_texture) {
        std::vector<uint32_t> zeros(w * h, 0);
        glBindTexture(GL_TEXTURE_2D, output_texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, zeros.data());
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void EaselCompute::reset() {
    destroy_texture();
    create_texture();
}

void EaselCompute::gui() {
    pal.RenderGui();
    
    ImGui::Text("Compute shader path active");
    ImGui::Text("Resolution: %d x %d", w, h);
    ImGui::Text("Window: %d x %d", ww, wh);
    ImGui::Text("Dispatch count: %d", dispatch_count);
    
    if (dispatch_count > 0) {
        float avg_time = static_cast<float>(total_dispatch_time_ms / dispatch_count);
        ImGui::Text("Avg dispatch time: %.3f ms", avg_time);
    }
    
    if (ImGui::Button("Reload Shader (TODO)")) {
        // TODO: Allow reloading shader from file
    }
}
