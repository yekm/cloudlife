#include "physarumgpu.hpp"

#ifndef __APPLE__

#include "easelcompute.h"
#include "imgui.h"
#include "imgui_elements.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

namespace {
constexpr float PI = 3.14159265358979323846f;
constexpr int MAX_SPECIES = 5;

const char* particle_shader = R"(
#version 430 core
layout(local_size_x = 256) in;
layout(std430, binding = 0) buffer Particles { vec4 particles[]; };
layout(r32ui, binding = 1) uniform coherent uimage2DArray deposits;
uniform sampler2DArray u_trails;
uniform int u_count;
uniform int u_species_count;
uniform vec2 u_size;
uniform int u_frame;
uniform float u_sensor_angle[5];
uniform float u_sensor_distance[5];
uniform float u_rotation_angle[5];
uniform float u_step_distance[5];
uniform float u_attraction[25];
float random(float n) { return fract(sin(n) * 43758.5453123); }
float sample_trails(vec2 position, int target) {
    ivec2 size = ivec2(u_size);
    ivec2 coordinate = ivec2(mod(floor(position) + u_size, u_size));
    float value = 0.0;
    for (int source = 0; source < u_species_count; ++source)
        value += texelFetch(u_trails, ivec3(coordinate, source), 0).r * u_attraction[target * 5 + source];
    return value;
}
void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= uint(u_count)) return;
    vec4 particle = particles[id];
    int species = int(particle.w + 0.5);
    float angle = u_sensor_angle[species];
    float distance = u_sensor_distance[species];
    float forward = sample_trails(particle.xy + vec2(cos(particle.z), sin(particle.z)) * distance, species);
    float left = sample_trails(particle.xy + vec2(cos(particle.z - angle), sin(particle.z - angle)) * distance, species);
    float right = sample_trails(particle.xy + vec2(cos(particle.z + angle), sin(particle.z + angle)) * distance, species);
    float turn = random(float(id) * 17.0 + float(u_frame) * 0.13);
    if (forward < left && forward < right) particle.z += turn < 0.5 ? -u_rotation_angle[species] : u_rotation_angle[species];
    else if (left > right) particle.z -= u_rotation_angle[species];
    else if (right > left) particle.z += u_rotation_angle[species];
    particle.xy = mod(particle.xy + vec2(cos(particle.z), sin(particle.z)) * u_step_distance[species] + u_size, u_size);
    imageAtomicAdd(deposits, ivec3(ivec2(particle.xy), species), 1u);
    particles[id] = particle;
})";

const char* trail_shader = R"(
#version 430 core
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
layout(binding = 0) uniform sampler2DArray u_source;
layout(r32ui, binding = 1) uniform coherent uimage2DArray deposits;
layout(r32f, binding = 2) uniform writeonly image2DArray u_destination;
uniform vec2 u_size;
uniform int u_species_count;
uniform float u_deposition_amount[5];
uniform float u_decay_factor[5];
float trail(ivec2 position, int layer) {
    ivec2 size = ivec2(u_size);
    return texelFetch(u_source, ivec3(ivec2((position.x + size.x) % size.x, (position.y + size.y) % size.y), layer), 0).r;
}
void main() {
    ivec2 position = ivec2(gl_GlobalInvocationID.xy);
    int species = int(gl_GlobalInvocationID.z);
    if (position.x >= int(u_size.x) || position.y >= int(u_size.y) || species >= u_species_count) return;
    float sum = 0.0;
    for (int y = -1; y <= 1; ++y) for (int x = -1; x <= 1; ++x) sum += trail(position + ivec2(x, y), species);
    uint deposited = imageLoad(deposits, ivec3(position, species)).r;
    imageStore(deposits, ivec3(position, species), uvec4(0u));
    imageStore(u_destination, ivec3(position, species), vec4(sum / 9.0 * u_decay_factor[species] + float(deposited) * u_deposition_amount[species]));
})";

const char* composite_shader = R"(
#version 430 core
layout(local_size_x = 16, local_size_y = 16) in;
layout(binding = 0) uniform sampler2DArray u_trails;
layout(rgba8, binding = 1) uniform writeonly image2D output_image;
uniform vec2 u_size;
uniform int u_species_count;
const vec3 colors[5] = vec3[5](vec3(49.0, 86.0, 250.0), vec3(31.0, 191.0, 255.0), vec3(70.0, 241.0, 255.0), vec3(25.0, 227.0, 171.0), vec3(129.0, 196.0, 0.0));
void main() {
    ivec2 position = ivec2(gl_GlobalInvocationID.xy);
    if (position.x >= int(u_size.x) || position.y >= int(u_size.y)) return;
    vec3 color = vec3(0.0);
    for (int species = 0; species < u_species_count; ++species) {
        float trail = texelFetch(u_trails, ivec3(position, species), 0).r;
        color += colors[species] / 255.0 * sqrt(min(1.0, trail / 40.0));
    }
    imageStore(output_image, position, vec4(min(color, vec3(1.0)), 1.0));
})";
}

PhysarumGPU::PhysarumGPU() : Art("PhysarumGPU") {
    useCompute();
    ecompute()->set_auto_dispatch(false);
    m_particle_program = compile_compute_program(particle_shader);
    m_trail_program = compile_compute_program(trail_shader);
    m_composite_program = compile_compute_program(composite_shader);
}

PhysarumGPU::~PhysarumGPU() { destroy_resources(); }

GLuint PhysarumGPU::compile_compute_program(const char* source) const {
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::fprintf(stderr, "PhysarumGPU shader: %s\n", log);
        glDeleteShader(shader);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::fprintf(stderr, "PhysarumGPU program: %s\n", log);
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

void PhysarumGPU::destroy_resources() {
    if (m_particle_buffer) glDeleteBuffers(1, &m_particle_buffer);
    if (m_trails[0] || m_trails[1]) glDeleteTextures(2, m_trails);
    if (m_deposits) glDeleteTextures(1, &m_deposits);
    if (m_particle_program) glDeleteProgram(m_particle_program);
    if (m_trail_program) glDeleteProgram(m_trail_program);
    if (m_composite_program) glDeleteProgram(m_composite_program);
    m_particle_buffer = m_deposits = m_particle_program = m_trail_program = m_composite_program = 0;
    m_trails[0] = m_trails[1] = 0;
}

void PhysarumGPU::create_resources() {
    if (m_width <= 0 || m_height <= 0) return;
    glGenTextures(2, m_trails);
    for (GLuint texture : m_trails) {
        glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
        glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, m_width, m_height, MAX_SPECIES);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }
    glGenTextures(1, &m_deposits);
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_deposits);
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32UI, m_width, m_height, MAX_SPECIES);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glGenBuffers(1, &m_particle_buffer);
    reset_simulation();
}

void PhysarumGPU::reset_simulation() {
    if (!m_particle_buffer || m_width <= 0 || m_height <= 0) return;
    std::mt19937 random(std::random_device{}());
    std::uniform_real_distribution<float> x(0.0f, static_cast<float>(m_width));
    std::uniform_real_distribution<float> y(0.0f, static_cast<float>(m_height));
    std::uniform_real_distribution<float> angle(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (int species = 0; species < m_species_count; ++species) {
        m_configs[species] = { (20.0f + unit(random) * 80.0f) * PI / 180.0f, 5.0f + unit(random) * 25.0f,
            (20.0f + unit(random) * 80.0f) * PI / 180.0f, 0.6f + unit(random) * 1.2f, 5.0f, 0.92f };
        for (int source = 0; source < m_species_count; ++source)
            m_attraction[species * MAX_SPECIES + source] = species == source ? 0.75f + unit(random) * 0.5f : -1.25f + unit(random) * 0.5f;
    }
    std::vector<float> particles(static_cast<size_t>(m_species_count) * m_particles_per_species * 4);
    for (int species = 0; species < m_species_count; ++species) for (int i = 0; i < m_particles_per_species; ++i) {
        size_t offset = static_cast<size_t>(species * m_particles_per_species + i) * 4;
        particles[offset] = x(random); particles[offset + 1] = y(random); particles[offset + 2] = angle(random); particles[offset + 3] = static_cast<float>(species);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_particle_buffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, particles.size() * sizeof(float), particles.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    std::vector<float> zero_trails(static_cast<size_t>(m_width) * m_height * MAX_SPECIES, 0.0f);
    std::vector<unsigned> zero_deposits(zero_trails.size(), 0);
    for (GLuint texture : m_trails) { glBindTexture(GL_TEXTURE_2D_ARRAY, texture); glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, m_width, m_height, MAX_SPECIES, GL_RED, GL_FLOAT, zero_trails.data()); }
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_deposits);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, m_width, m_height, MAX_SPECIES, GL_RED_INTEGER, GL_UNSIGNED_INT, zero_deposits.data());
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
    m_current_trail = 0;
}

void PhysarumGPU::resize(int width, int height) {
    default_resize(width, height);
    m_width = width; m_height = height;
    destroy_resources();
    m_particle_program = compile_compute_program(particle_shader);
    m_trail_program = compile_compute_program(trail_shader);
    m_composite_program = compile_compute_program(composite_shader);
    create_resources();
}

void PhysarumGPU::shuffle() { reset_simulation(); }

bool PhysarumGPU::render(uint32_t*) {
    if (!m_particle_program || !m_trail_program || !m_composite_program || !m_particle_buffer) return false;
    const int particle_count = m_species_count * m_particles_per_species;
    float sensor_angle[MAX_SPECIES], sensor_distance[MAX_SPECIES], rotation_angle[MAX_SPECIES], step_distance[MAX_SPECIES], deposition[MAX_SPECIES], decay[MAX_SPECIES];
    for (int i = 0; i < MAX_SPECIES; ++i) { sensor_angle[i] = m_configs[i].sensor_angle; sensor_distance[i] = m_configs[i].sensor_distance; rotation_angle[i] = m_configs[i].rotation_angle; step_distance[i] = m_configs[i].step_distance; deposition[i] = m_configs[i].deposition_amount; decay[i] = m_configs[i].decay_factor; }
    for (int step = 0; step < m_steps_per_frame; ++step) {
        glUseProgram(m_particle_program);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_particle_buffer);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, m_trails[m_current_trail]);
        glUniform1i(glGetUniformLocation(m_particle_program, "u_trails"), 0);
        glUniform1i(glGetUniformLocation(m_particle_program, "u_count"), particle_count);
        glUniform1i(glGetUniformLocation(m_particle_program, "u_species_count"), m_species_count);
        glUniform2f(glGetUniformLocation(m_particle_program, "u_size"), static_cast<float>(m_width), static_cast<float>(m_height));
        glUniform1i(glGetUniformLocation(m_particle_program, "u_frame"), static_cast<int>(frame_number) + step);
        glUniform1fv(glGetUniformLocation(m_particle_program, "u_sensor_angle[0]"), MAX_SPECIES, sensor_angle);
        glUniform1fv(glGetUniformLocation(m_particle_program, "u_sensor_distance[0]"), MAX_SPECIES, sensor_distance);
        glUniform1fv(glGetUniformLocation(m_particle_program, "u_rotation_angle[0]"), MAX_SPECIES, rotation_angle);
        glUniform1fv(glGetUniformLocation(m_particle_program, "u_step_distance[0]"), MAX_SPECIES, step_distance);
        glUniform1fv(glGetUniformLocation(m_particle_program, "u_attraction[0]"), MAX_SPECIES * MAX_SPECIES, m_attraction);
        glBindImageTexture(1, m_deposits, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
        glDispatchCompute((particle_count + 255) / 256, 1, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        const int next = 1 - m_current_trail;
        glUseProgram(m_trail_program);
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, m_trails[m_current_trail]);
        glUniform1i(glGetUniformLocation(m_trail_program, "u_source"), 0);
        glUniform2f(glGetUniformLocation(m_trail_program, "u_size"), static_cast<float>(m_width), static_cast<float>(m_height));
        glUniform1i(glGetUniformLocation(m_trail_program, "u_species_count"), m_species_count);
        glUniform1fv(glGetUniformLocation(m_trail_program, "u_deposition_amount[0]"), MAX_SPECIES, deposition);
        glUniform1fv(glGetUniformLocation(m_trail_program, "u_decay_factor[0]"), MAX_SPECIES, decay);
        glBindImageTexture(1, m_deposits, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
        glBindImageTexture(2, m_trails[next], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32F);
        glDispatchCompute((m_width + 15) / 16, (m_height + 15) / 16, m_species_count);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
        m_current_trail = next;
    }
    glUseProgram(m_composite_program);
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D_ARRAY, m_trails[m_current_trail]);
    glUniform1i(glGetUniformLocation(m_composite_program, "u_trails"), 0);
    glUniform2f(glGetUniformLocation(m_composite_program, "u_size"), static_cast<float>(m_width), static_cast<float>(m_height));
    glUniform1i(glGetUniformLocation(m_composite_program, "u_species_count"), m_species_count);
    glBindImageTexture(1, ecompute()->get_output_texture(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glDispatchCompute((m_width + 15) / 16, (m_height + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    return false;
}

bool PhysarumGPU::render_gui() {
    bool reset = ScrollableSliderInt("particles / species", &m_particles_per_species, 1000, 100000, "%d", 1000);
    ScrollableSliderInt("steps / frame", &m_steps_per_frame, 1, 8, "%d", 1);
    reset |= ScrollableSliderInt("species", &m_species_count, 1, MAX_SPECIES, "%d", 1);
    if (reset) { m_selected_species = std::min(m_selected_species, m_species_count - 1); reset_simulation(); }
    ImGui::Separator();
    ScrollableSliderInt("edit species", &m_selected_species, 0, m_species_count - 1, "%d", 1);
    Config& config = m_configs[m_selected_species];
    float sensor_angle = config.sensor_angle * 180.0f / PI, rotation_angle = config.rotation_angle * 180.0f / PI;
    if (ScrollableSliderFloat("sensor angle", &sensor_angle, 0.0f, 120.0f, "%.1f deg", 1.0f)) config.sensor_angle = sensor_angle * PI / 180.0f;
    ScrollableSliderFloat("sensor distance", &config.sensor_distance, 0.0f, 64.0f, "%.1f", 1.0f);
    if (ScrollableSliderFloat("rotation angle", &rotation_angle, 0.0f, 120.0f, "%.1f deg", 1.0f)) config.rotation_angle = rotation_angle * PI / 180.0f;
    ScrollableSliderFloat("step distance", &config.step_distance, 0.2f, 2.0f, "%.2f", 0.1f);
    ScrollableSliderFloat("deposition amount", &config.deposition_amount, 0.0f, 10.0f, "%.2f", 0.5f);
    ScrollableSliderFloat("decay factor", &config.decay_factor, 0.0f, 1.0f, "%.3f", 0.01f);
    ImGui::Separator(); ImGui::Text("Attraction for species %d", m_selected_species + 1);
    for (int source = 0; source < m_species_count; ++source) { ImGui::PushID(source); ScrollableSliderFloat("from species", &m_attraction[m_selected_species * MAX_SPECIES + source], -2.0f, 2.0f, "%.2f", 0.05f); ImGui::SameLine(); ImGui::Text("%d", source + 1); ImGui::PopID(); }
    return false;
}

#endif
