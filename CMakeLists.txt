cmake_minimum_required(VERSION 3.10)

project(cloudlife 
    VERSION 1.0.0
    DESCRIPTION "Xscreensaver hacks and other beautiful programs with Dear ImGui"
    LANGUAGES C CXX
)

# Export compile commands for clangd/language servers
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Verbose makefile output (use -DCMAKE_VERBOSE_MAKEFILE=ON to enable)
option(CMAKE_VERBOSE_MAKEFILE "Enable verbose makefile output" ON)

# Prevent in-source builds
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not allowed. Please create a build directory and run cmake from there.")
endif()

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find packages
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/")
find_package(GLFW3 REQUIRED)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

# Suppress OpenGL deprecation warnings on macOS
add_compile_definitions(GL_SILENCE_DEPRECATION)

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    #GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# ImGui library
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC)

target_sources(imgui
    PRIVATE
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

target_include_directories(imgui
    PUBLIC 
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

# Main library with all sources
set(CLOUDS_SOURCES
    # Core art infrastructure
    src/core/art.cpp
    src/core/artfactory.cpp
    
    # Utilities
    src/utils/settings.cpp
    src/utils/imgui_elements.cpp
    src/utils/timer.cpp
    src/utils/random.c
    src/utils/screenshot.cpp
    
    # Rendering system
    src/render/easelplane.cpp
    src/render/easelvertex.cpp
    src/render/easelvertex3d.cpp
    src/render/vertexbuffer.cpp
    
    # Art implementations
    src/arts/test3d.cpp
    src/arts/cloudlife.cpp
    src/arts/mtron.cpp
    src/arts/ifs.cpp
    src/arts/vermiculate.cpp
    src/arts/discrete.cpp
    src/arts/thornbird.cpp
    src/arts/rdbomb.cpp
    src/arts/acidwarp/acidwarp.c
    src/arts/acidwarp/lut.c
    src/arts/acidwarp/palinit.c
    src/arts/acidwarp/bit_map.c
    src/arts/acidwarp/rolnfade.c
    src/arts/acidwarp/warp_text.c
    src/arts/acidwarp.cpp
    src/arts/acidworm.cpp
    src/arts/hopalong.cpp
    src/arts/attractor.cpp
)

# Compute shaders require OpenGL 4.3+ (not available on macOS)
if(NOT APPLE)
    list(APPEND CLOUDS_SOURCES
        src/render/easelcompute.cpp
        src/arts/plasmacompute.cpp
        src/arts/sphereofcubes.cpp
    )
endif()

add_library(clouds OBJECT ${CLOUDS_SOURCES})

target_include_directories(clouds 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/utils
        ${CMAKE_SOURCE_DIR}/src/render
        ${CMAKE_SOURCE_DIR}/src/arts
        ${CMAKE_SOURCE_DIR}/external/colormap-shaders/include
        ${CMAKE_SOURCE_DIR}/external/stb
)

target_link_libraries(clouds imgui glfw glm OpenGL::GL)

# Main executable
add_executable(cloudlife main.cpp $<TARGET_OBJECTS:clouds>)

target_include_directories(cloudlife 
    PRIVATE 
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/core
        ${CMAKE_SOURCE_DIR}/src/utils
        ${CMAKE_SOURCE_DIR}/src/render
        ${CMAKE_SOURCE_DIR}/src/arts
        ${CMAKE_SOURCE_DIR}/external/colormap-shaders/include
        ${CMAKE_SOURCE_DIR}/external/stb
)

target_link_libraries(cloudlife imgui glfw glm OpenGL::GL)

# Platform-specific libraries for macOS
if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(CORE_VIDEO_FRAMEWORK CoreVideo)

    target_link_libraries(cloudlife PRIVATE
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${CORE_FOUNDATION_FRAMEWORK}
        ${CORE_VIDEO_FRAMEWORK}
    )
endif()

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(clouds PRIVATE -Wall -Wextra -Wno-unknown-pragmas -Wno-unused-parameter -Wno-pragmas)
    target_compile_options(cloudlife PRIVATE -Wall -Wextra -Wno-unknown-pragmas -Wno-unused-parameter -Wno-pragmas)
endif()

# Release optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(clouds PRIVATE -O3 -march=native -flto=auto)
    target_compile_options(cloudlife PRIVATE -O3 -march=native -flto=auto)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    target_compile_options(clouds PRIVATE -O3 -march=native -g)
    target_compile_options(cloudlife PRIVATE -O3 -march=native -g)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(clouds PRIVATE -O0 -ggdb3 -fsanitize=address)
    target_compile_options(cloudlife PRIVATE -O0 -ggdb3 -fsanitize=address)
    target_link_options(cloudlife PRIVATE -fsanitize=address)
endif()

# Output directory
set_target_properties(cloudlife PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Install
install(TARGETS cloudlife DESTINATION bin)
