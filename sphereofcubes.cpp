#include "sphereofcubes.h"
#include "easelcompute.h"

#include "imgui_elements.h"

// Compute shader for sphere of cubes effect
const char* SphereOfCubes::sphere_compute_shader_base = R"(
    #version 430 core
    
    layout(local_size_x = 16, local_size_y = 16) in;
    
    layout(rgba8, binding = 0) uniform writeonly image2D output_image;
    
    uniform vec2 u_resolution;
    uniform float u_time;
    uniform float u_cube_width;
    uniform float u_sphere_radius;
    uniform float u_camera_distance;
    uniform float u_camera_height;
    uniform float u_rotation_speed;
    uniform float u_aperture;
    
    struct Camera {
        vec3 Obs;
        vec3 View;
        vec3 Up;
        vec3 Horiz;
        float H;
        float W;
        float z;
    };
    
    struct Ray {
        vec3 Origin;
        vec3 Dir;
    };
    
    Camera camera(in vec3 Obs, in vec3 LookAt, in float aperture) {
        Camera C;
        C.Obs = Obs;
        C.View = normalize(LookAt - Obs);
        C.Horiz = normalize(cross(vec3(0.0, 0.0, 1.0), C.View));
        C.Up = cross(C.View, C.Horiz);
        C.W = u_resolution.x;
        C.H = u_resolution.y;
        C.z = (C.H / 2.0) / tan((aperture * 3.1415 / 180.0) / 2.0);
        return C;
    }
    
    Ray launch(in Camera C, in vec2 XY) {
        return Ray(
            C.Obs,
            normalize(C.z * C.View + (XY.x - C.W / 2.0) * C.Horiz + (XY.y - C.H / 2.0) * C.Up)
        );
    }
    
    struct Sphere {
        vec3 Center;
        float R;
    };
    
    bool intersect_sphere(in Ray R, in Sphere S, out float t, out float t2) {
        vec3 CO = R.Origin - S.Center;
        float a = dot(R.Dir, R.Dir);
        float b = 2.0 * dot(R.Dir, CO);
        float c = dot(CO, CO) - S.R * S.R;
        float delta = b * b - 4.0 * a * c;
        if (delta < 0.0) {
            return false;
        }
        t = (-b - sqrt(delta)) / (2.0 * a);
        t2 = (-b + sqrt(delta)) / (2.0 * a);
        return true;
    }
    
    bool step_forward(in Ray R, inout float t, inout vec3 roundpoint, out int coord, in float max_t, in float cubeWidth, in float cubesRad) {
        vec3 point = R.Origin + t * R.Dir;
        vec3 signDir = sign(R.Dir);
        vec3 params = (roundpoint - point + 0.5 * signDir * cubeWidth) / R.Dir;
        
        if (params.x < params.y) {
            if (params.x < params.z) { coord = 0; }
            else { coord = 2; }
        }
        else {
            if (params.y < params.z) { coord = 1; }
            else { coord = 2; }
        }
        
        t += params[coord];
        vec3 move = vec3(0.0, 0.0, 0.0);
        move[coord] += cubeWidth;
        roundpoint += signDir * move;
        
        if (length(roundpoint) < cubesRad) {
            roundpoint -= signDir * move;
            return false;
        }
        if (t > max_t) {
            coord = 3;
            return false;
        }
        
        return true;
    }
    
    float mysmoothstep(in float x) {
        float t = clamp(x, 0.0, 1.0);
        t = 1.0 - pow((1.0 - t), 1.5);
        return t * t * (3.0 - 2.0 * t);
    }
    
    void main() {
        ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);
        
        if (pixel_coords.x >= int(u_resolution.x) || pixel_coords.y >= int(u_resolution.y))
            return;
        
        float cubeWidth = u_cube_width;
        float sphereRad = u_sphere_radius;
        float cubesRad = sphereRad - cubeWidth * 0.5 * sqrt(3.0);
        
        vec3 red = vec3(1.0, 0.0, 0.0);
        vec3 green = vec3(0.0, 1.0, 0.0);
        vec3 blue = vec3(0.0, 0.0, 1.0);
        
        float beta = 3.14159 / 4.0 + u_rotation_speed * u_time;
        float s = sin(beta);
        float c = cos(beta);
        
        Camera C = camera(
            vec3(u_camera_distance * c, u_camera_distance * s, u_camera_height),
            vec3(0.0, 0.0, 0.0),
            u_aperture
        );
        
        Ray R = launch(C, vec2(float(pixel_coords.x), float(pixel_coords.y)));
        Sphere S = Sphere(vec3(0.0, 0.0, 0.0), sphereRad);
        
        vec4 fragColor = vec4(0.1, 0.1, 0.1, 1.0);
        
        float t;
        float max_t;
        int coord;
        
        vec3 col = vec3(0.0, 0.0, 0.0);
        if (intersect_sphere(R, S, t, max_t)) {
            vec3 point = R.Origin + t * R.Dir;
            vec3 roundpoint = round(point / cubeWidth) * cubeWidth;
            
            bool cont = true;
            for (int i = 0; i <= 50; i++) {
                cont = step_forward(R, t, roundpoint, coord, max_t, cubeWidth, cubesRad);
                if (cont == false) { break; }
            }
            
            if (coord <= 2) {
                col[coord] = 1.0;
                
                vec3 hit_point = R.Origin + t * R.Dir;
                vec3 signOctant = -sign(hit_point);
                hit_point -= roundpoint;
                hit_point *= signOctant;
                hit_point /= cubeWidth;
                
                float brightness = 1.0;
                float d = 0.3;
                
                vec3 move1 = vec3(0.0, 0.0, 0.0);
                move1[(coord + 1) % 3] += cubeWidth;
                vec3 neighbour1 = signOctant * roundpoint + move1;
                vec3 move2 = vec3(0.0, 0.0, 0.0);
                move2[(coord + 2) % 3] += cubeWidth;
                vec3 neighbour2 = signOctant * roundpoint + move2;
                vec3 move3 = move1 + move2;
                vec3 neighbour3 = signOctant * roundpoint + move3;
                bool n1solid = (length(neighbour1) < cubesRad);
                bool n2solid = (length(neighbour2) < cubesRad);
                bool n3solid = (length(neighbour3) < cubesRad);
                
                if (n1solid) {
                    brightness *= (1.0 - d) + d * mysmoothstep(0.5 - hit_point[(coord + 1) % 3]);
                }
                if (n2solid) {
                    brightness *= (1.0 - d) + d * mysmoothstep(0.5 - hit_point[(coord + 2) % 3]);
                }
                
                if (n3solid && (!n1solid && !n2solid)) {
                    float s1 = mysmoothstep(0.5 - hit_point[(coord + 1) % 3]);
                    float s2 = mysmoothstep(0.5 - hit_point[(coord + 2) % 3]);
                    float foo = 1.0 - (1.0 - s1) * (1.0 - s2);
                    brightness *= (1.0 - d) + d * foo;
                }
                
                col *= brightness;
                fragColor = vec4(col, 1.0);
            }
        }
        
        imageStore(output_image, pixel_coords, fragColor);
    }
)";

SphereOfCubes::SphereOfCubes()
    : Art("Sphere of Cubes") {
    useCompute();
    init_shader();
    update_uniform_callback();
}

void SphereOfCubes::init_shader() {
    ecompute()->set_compute_shader(sphere_compute_shader_base);
}

void SphereOfCubes::update_uniform_callback() {
    ecompute()->set_uniform_callback([this](GLuint program) {
        GLint cube_width_loc = glGetUniformLocation(program, "u_cube_width");
        GLint sphere_radius_loc = glGetUniformLocation(program, "u_sphere_radius");
        GLint camera_distance_loc = glGetUniformLocation(program, "u_camera_distance");
        GLint camera_height_loc = glGetUniformLocation(program, "u_camera_height");
        GLint rotation_speed_loc = glGetUniformLocation(program, "u_rotation_speed");
        GLint aperture_loc = glGetUniformLocation(program, "u_aperture");
        
        if (cube_width_loc >= 0) glUniform1f(cube_width_loc, cube_width);
        if (sphere_radius_loc >= 0) glUniform1f(sphere_radius_loc, sphere_radius);
        if (camera_distance_loc >= 0) glUniform1f(camera_distance_loc, camera_distance);
        if (camera_height_loc >= 0) glUniform1f(camera_height_loc, camera_height);
        if (rotation_speed_loc >= 0) glUniform1f(rotation_speed_loc, rotation_speed);
        if (aperture_loc >= 0) glUniform1f(aperture_loc, aperture);
    });
}

bool SphereOfCubes::render(uint32_t *p) {
    return true;
}

bool SphereOfCubes::render_gui() {
    bool up = false;
    
    up |= ScrollableSliderFloat("Cube Width", &cube_width, 0.05f, 0.5f, "%.3f", 0.01f);
    up |= ScrollableSliderFloat("Sphere Radius", &sphere_radius, 0.5f, 2.0f, "%.2f", 0.1f);
    up |= ScrollableSliderFloat("Camera Distance", &camera_distance, 1.0f, 5.0f, "%.2f", 0.1f);
    up |= ScrollableSliderFloat("Camera Height", &camera_height, 0.0f, 3.0f, "%.2f", 0.1f);
    up |= ScrollableSliderFloat("Rotation Speed", &rotation_speed, -1.0f, 1.0f, "%.2f", 0.05f);
    up |= ScrollableSliderFloat("Aperture (FOV)", &aperture, 20.0f, 120.0f, "%.1f", 5.0f);
    
    return false;
}

void SphereOfCubes::resize(int _w, int _h) {
    default_resize(_w, _h);
}
