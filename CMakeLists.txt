cmake_minimum_required(VERSION 3.16.3)

project(imgui-cmake LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set (CMAKE_C_FLAGS_RELEASE "-O3 -march=native -flto=auto -Wno-pragmas")
set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})

set (CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -march=native -g -Wno-pragmas")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})

set (CMAKE_C_FLAGS_DEBUG "-O0 -ggdb3 -fsanitize=address -Wno-pragmas")
set (CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set (CMAKE_VERBOSE_MAKEFILE TRUE)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
# https://github.com/JoeyDeVries/LearnOpenGL/tree/master/cmake/modules
find_package(GLFW3 REQUIRED)
set (OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)

add_definitions(-DGL_SILENCE_DEPRECATION)

# https://github.com/Pesc0/imgui-cmake
######### IMGUI #######################

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
add_library(IMGUI STATIC)

target_sources( IMGUI
                PRIVATE
                    ${IMGUI_DIR}/imgui_demo.cpp
                    ${IMGUI_DIR}/imgui_draw.cpp
                    ${IMGUI_DIR}/imgui_tables.cpp
                    ${IMGUI_DIR}/imgui_widgets.cpp
                    ${IMGUI_DIR}/imgui.cpp

                PRIVATE
                    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
                    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
#                    ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
                )

target_include_directories( IMGUI
                            PUBLIC ${IMGUI_DIR}
                            PUBLIC ${IMGUI_DIR}/backends
                            )

#target_link_libraries(IMGUI PUBLIC ${CMAKE_DL_LIBS})
#######################################################

set(CLOUDS_SOURCES
    art.cpp
    artfactory.cpp
    settings.cpp
    imgui_elements.cpp
    easelplane.cpp
    easelvertex.cpp
    random.c
    cloudlife.cpp
    mtron.cpp
    timer.cpp
    ifs.cpp
    vermiculate.cpp
    discrete.cpp
    thornbird.cpp
    rdbomb.cpp
    acidwarp/acidwarp.c
    acidwarp/lut.c
    acidwarp/palinit.c
    acidwarp/bit_map.c
    acidwarp/rolnfade.c
    acidwarp/warp_text.c
    acidwarp.cpp
    acidworm.cpp
    hopalong.cpp
    attractor.cpp
)

# Compute shaders require OpenGL 4.3+, not available on macOS
if(NOT APPLE)
    list(APPEND CLOUDS_SOURCES
        easelcompute.cpp
        plasmacompute.cpp
    )
endif()

add_library(clouds OBJECT ${CLOUDS_SOURCES})

target_include_directories(clouds PRIVATE .)
target_include_directories(clouds PRIVATE colormap-shaders/include)

target_link_libraries(clouds IMGUI glfw GL)
#set_property(TARGET clouds PROPERTY CXX_STANDARD 17)

add_executable(cloudlife main.cpp $<TARGET_OBJECTS:clouds>)
target_include_directories(cloudlife PRIVATE colormap-shaders/include)
#add_executable(cloudtest test.cpp $<TARGET_OBJECTS:clouds>)


if(APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(IOKIT_FRAMEWORK IOKit)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    find_library(CORE_VIDEO_FRAMEWORK CoreVideo)

    set(GLFW_DEPENDENCIES 
        ${COCOA_FRAMEWORK} 
        ${IOKIT_FRAMEWORK} 
        ${CORE_FOUNDATION_FRAMEWORK} 
        ${CORE_VIDEO_FRAMEWORK}
    )
endif()

############################################################


target_link_libraries(cloudlife IMGUI ${GLFW3_LIBRARY} ${OPENGL_LIBRARIES} ${GLFW_DEPENDENCIES})

#target_link_libraries(cloudtest IMGUI ${GLFW3_LIBRARY} ${OPENGL_LIBRARIES})
set_target_properties(cloudlife PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
#set_target_properties(cloudtest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

#include(CTest)

#add_test(NAME CloudTest1 COMMAND cloudtest -f 1024 -i 2 -r 512)
#add_test(NAME CloudTest2 COMMAND cloudtest -f 32 -i 8 -r 16)
