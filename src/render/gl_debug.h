#pragma once

// Must define these before including GLFW to get OpenGL extension prototypes

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

// OpenGL debug callback setup
// Call init_gl_debug() after creating OpenGL context to enable debug output

static const char* get_debug_source(GLenum source) {
    switch (source) {
        case GL_DEBUG_SOURCE_API: return "API";
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: return "Window System";
        case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader Compiler";
        case GL_DEBUG_SOURCE_THIRD_PARTY: return "Third Party";
        case GL_DEBUG_SOURCE_APPLICATION: return "Application";
        case GL_DEBUG_SOURCE_OTHER: return "Other";
        default: return "Unknown";
    }
}

static const char* get_debug_type(GLenum type) {
    switch (type) {
        case GL_DEBUG_TYPE_ERROR: return "Error";
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated Behavior";
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: return "Undefined Behavior";
        case GL_DEBUG_TYPE_PORTABILITY: return "Portability";
        case GL_DEBUG_TYPE_PERFORMANCE: return "Performance";
        case GL_DEBUG_TYPE_MARKER: return "Marker";
        case GL_DEBUG_TYPE_PUSH_GROUP: return "Push Group";
        case GL_DEBUG_TYPE_POP_GROUP: return "Pop Group";
        case GL_DEBUG_TYPE_OTHER: return "Other";
        default: return "Unknown";
    }
}

static const char* get_debug_severity(GLenum severity) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH: return "HIGH";
        case GL_DEBUG_SEVERITY_MEDIUM: return "MEDIUM";
        case GL_DEBUG_SEVERITY_LOW: return "LOW";
        case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTIFICATION";
        default: return "UNKNOWN";
    }
}

static void GLAPIENTRY gl_debug_callback(GLenum source, GLenum type, GLuint id,
                                         GLenum severity, GLsizei length,
                                         const GLchar* message, const void* userParam) {
    // Ignore notifications in non-debug builds
    #ifndef NDEBUG
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
        return;
    }
    #else
    // In release builds, only log high severity errors
    if (severity != GL_DEBUG_SEVERITY_HIGH) {
        return;
    }
    #endif

    fprintf(stderr, "\n=== GL DEBUG ===\n");
    fprintf(stderr, "Source: %s\n", get_debug_source(source));
    fprintf(stderr, "Type: %s\n", get_debug_type(type));
    fprintf(stderr, "Severity: %s\n", get_debug_severity(severity));
    fprintf(stderr, "ID: %u\n", id);
    fprintf(stderr, "Message: %s\n", message);
    fprintf(stderr, "================\n\n");

    // Break on high severity errors in debug builds
    #ifndef NDEBUG
    if (severity == GL_DEBUG_SEVERITY_HIGH || type == GL_DEBUG_TYPE_ERROR) {
        // You can set a breakpoint here
        // raise(SIGTRAP); // Uncomment to break into debugger
    }
    #endif
}

static void init_gl_debug() {
    GLint flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        fprintf(stderr, "Warning: OpenGL context not created with debug flag.\n");
        fprintf(stderr, "Debug output may not be available or reliable.\n");
    }

    // Check if debug output is supported
    if (glDebugMessageCallback) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // Synchronous for accurate stack traces
        glDebugMessageCallback(gl_debug_callback, nullptr);
        
        // Optionally filter messages
        // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
        
        fprintf(stderr, "OpenGL debug callback registered successfully.\n");
        
        // Test the debug callback
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_MARKER,
                            0, GL_DEBUG_SEVERITY_NOTIFICATION, -1,
                            "Debug callback initialized");
    } else {
        fprintf(stderr, "Warning: glDebugMessageCallback not available (requires OpenGL 4.3+).\n");
    }
}

