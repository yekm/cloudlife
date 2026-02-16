#pragma once

#include <GLFW/glfw3.h>

// RAII wrappers for common OpenGL state management
// These are compatible with both legacy and modern OpenGL

// Scoped GL enable/disable helper
// Usage: 
//   {
//     GLScopedEnable blend(GL_BLEND);  // Enables GL_BLEND if not already enabled
//     // ... do rendering ...
//   } // GL_BLEND restored to previous state on scope exit
//
//   {
//     GLScopedDisable depth(GL_DEPTH_TEST);  // Disables GL_DEPTH_TEST if enabled
//     // ... do rendering ...
//   } // GL_DEPTH_TEST restored to previous state

class GLScopedEnable {
    GLenum cap;
    bool wasEnabled;

public:
    explicit GLScopedEnable(GLenum capability)
        : cap(capability)
    {
        wasEnabled = glIsEnabled(capability);
        if (!wasEnabled) {
            glEnable(cap);
        }
    }

    ~GLScopedEnable() {
        if (!wasEnabled) {
            glDisable(cap);
        }
    }

    GLScopedEnable(const GLScopedEnable&) = delete;
    GLScopedEnable& operator=(const GLScopedEnable&) = delete;
};

class GLScopedDisable {
    GLenum cap;
    bool wasEnabled;

public:
    explicit GLScopedDisable(GLenum capability)
        : cap(capability)
    {
        wasEnabled = glIsEnabled(capability);
        if (wasEnabled) {
            glDisable(cap);
        }
    }

    ~GLScopedDisable() {
        if (wasEnabled) {
            glEnable(cap);
        }
    }

    GLScopedDisable(const GLScopedDisable&) = delete;
    GLScopedDisable& operator=(const GLScopedDisable&) = delete;
};

// Scoped viewport setter
// Usage: GLScopedViewport vp(x, y, width, height);
class GLScopedViewport {
    GLint oldViewport[4];

public:
    GLScopedViewport(GLint x, GLint y, GLsizei width, GLsizei height) {
        glGetIntegerv(GL_VIEWPORT, oldViewport);
        glViewport(x, y, width, height);
    }

    ~GLScopedViewport() {
        glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    }

    GLScopedViewport(const GLScopedViewport&) = delete;
    GLScopedViewport& operator=(const GLScopedViewport&) = delete;
};

// Scoped scissor box setter
class GLScopedScissor {
    GLint oldScissor[4];

public:
    GLScopedScissor(GLint x, GLint y, GLsizei width, GLsizei height) {
        glGetIntegerv(GL_SCISSOR_BOX, oldScissor);
        glScissor(x, y, width, height);
    }

    ~GLScopedScissor() {
        glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
    }

    GLScopedScissor(const GLScopedScissor&) = delete;
    GLScopedScissor& operator=(const GLScopedScissor&) = delete;
};

// Scoped depth mask setter
class GLScopedDepthMask {
    GLboolean oldMask;

public:
    explicit GLScopedDepthMask(GLboolean mask) {
        glGetBooleanv(GL_DEPTH_WRITEMASK, &oldMask);
        glDepthMask(mask);
    }

    ~GLScopedDepthMask() {
        glDepthMask(oldMask);
    }

    GLScopedDepthMask(const GLScopedDepthMask&) = delete;
    GLScopedDepthMask& operator=(const GLScopedDepthMask&) = delete;
};

// Helper macros for common patterns
#define GL_ENABLE_FOR_SCOPE(cap) GLScopedEnable _gl_enable_##__LINE__(cap)
#define GL_DISABLE_FOR_SCOPE(cap) GLScopedDisable _gl_disable_##__LINE__(cap)
#define GL_VIEWPORT_FOR_SCOPE(x, y, w, h) GLScopedViewport _gl_viewport_##__LINE__(x, y, w, h)
