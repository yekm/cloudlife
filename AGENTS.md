# AGENTS.md

Guide for AI coding agents working on the cloudlife repository.

## Project Overview

Cloudlife is a screensaver/visualization application using Dear ImGui, OpenGL, and GLFW. It ports classic xscreensaver hacks to a modern OpenGL-based renderer with an interactive GUI.

- **Language**: C++17 with some C components
- **Build System**: CMake (minimum version 3.16)
- **Main Executable**: `cloudlife`

## Build Commands

### Debug Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_VERBOSE_MAKEFILE=OFF
cmake --build build -j$(nproc)
```

## Code Style Guidelines

### File Organization
- Headers: Use `#pragma once` (preferred) or include guards
- Extensions: `.hpp` for C++ headers, `.h` for C headers, `.cpp` for C++ source
- Include order:
  1. Corresponding header (for .cpp files)
  2. Project headers (quotes)
  3. Third-party headers (quotes for imgui, angle brackets for system libs)
  4. Standard library headers (angle brackets)

### Naming Conventions
- **Classes**: `PascalCase` (e.g., `Art`, `EaselPlane`, `Cloudlife`)
- **Functions**: `snake_case` (e.g., `resized()`, `draw_field()`)
- **Variables**: `snake_case` (e.g., `frame_number`, `tex_w`)
- **Member variables**: 
  - `m_` prefix for class members (e.g., `m_name`)
  - `f->` for field struct members (e.g., `f->width`)
- **Private virtual methods**: `camelCase` or `snake_case`
- **Constants**: `UPPER_SNAKE_CASE` for macros, `camelCase` or `PascalCase` for constexpr
- **Template parameters**: `PascalCase`

### Formatting
- Indentation: 4 spaces (no tabs)
- Line endings: Unix-style (LF)
- Opening braces:
  - Same line for control flow: `if (x) {`
  - New line for class/function definitions
- Maximum line length: ~100-120 characters
- Pointer/reference alignment: `Type* ptr`, `Type& ref`

### C++ Style
- Use `override` for all overridden virtual methods
- Prefer `std::unique_ptr` for ownership
- Use `auto` when type is obvious or very long
- Use range-based for loops: `for (const auto& item : container)`
- Prefer `nullptr` over `NULL` or `0`
- Use `constexpr` for compile-time constants
- Prefer `using` over `typedef`

### OpenGL/Graphics
- Use GLAD loader for OpenGL function pointers
- Always check for OpenGL errors in debug builds
- Use PBOs for pixel transfers
- Prefer compute shaders on OpenGL 4.3+ (Linux only, not macOS)

### Error Handling
- Use exceptions sparingly (not used in this codebase)
- Check OpenGL state after operations
- Use early returns to reduce nesting
- Log errors to stderr: `fprintf(stderr, "...\n")`

## Project Structure

```
src/
  core/       - Base Art class and factory
  render/     - Easel rendering backends (Plane, Vertex, Compute, 3D)
  arts/       - Individual screensaver implementations
  utils/      - Settings, timers, random, UI elements
external/     - Third-party dependencies (imgui, stb, colormap-shaders)
cmake/        - CMake modules (FindGLFW3.cmake)
```

## Creating New Art

To add a new screensaver:

1. Create `src/arts/yourart.hpp` extending `Art` class
2. Create `src/arts/yourart.cpp` implementing virtual methods
3. Register in `src/core/artfactory.cpp` using `add_art<YourArt>("Name")`
4. Add files to `CLOUDS_SOURCES` in `CMakeLists.txt`
5. Rebuild: `cmake --build build -j$(nproc)`

## Dependencies

- **GLFW3** - Window/input management
- **OpenGL** - Graphics (3.0+ on Linux, 3.2+ Core on macOS)
- **GLM** - Math library (fetched by CMake)
- **Dear ImGui** - Immediate mode GUI (in external/)

## Platform Notes

- **Linux**: Full OpenGL 4.3+ support including compute shaders
- **macOS**: OpenGL 3.2 Core only, no compute shaders
- **Windows**: Not currently supported but should work with minor CMake adjustments

## IDE/Editor Setup

The project generates `compile_commands.json` for language servers. Symlink it:
```bash
ln -s build/compile_commands.json compile_commands.json
```

## Git

- Use conventional verbose commit messages
- Add information about the agent: model, version, software used, etc
- Don't commit build artifacts or `compile_commands.json`
- Keep external dependencies in `external/` as-is (vendored)
