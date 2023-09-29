cmake_minimum_required(VERSION 3.16.3)

project(imgui-cmake LANGUAGES C CXX)


set (CMAKE_C_FLAGS_RELEASE "-O3 -march=native -flto=auto")
set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set (CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -march=native -g")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
set (CMAKE_C_FLAGS_DEBUG "-O0 -ggdb3 -fsanitize=address")
set (CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set (CMAKE_VERBOSE_MAKEFILE TRUE)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")
# https://github.com/JoeyDeVries/LearnOpenGL/tree/master/cmake/modules
find_package(GLFW3 REQUIRED)
set (OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)


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

add_subdirectory(colormap)

add_library(clouds OBJECT
    art.cpp
    artfactory.cpp
    settings.cpp
    imgui_elements.cpp
    vertexbuffer.cpp
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

target_include_directories(clouds PRIVATE .)

target_link_libraries(clouds IMGUI glfw GL colormap)
#set_property(TARGET clouds PROPERTY CXX_STANDARD 17)

add_executable(cloudlife main.cpp $<TARGET_OBJECTS:clouds>)
add_executable(cloudtest test.cpp $<TARGET_OBJECTS:clouds>)
target_link_libraries(cloudlife IMGUI ${GLFW3_LIBRARY} ${OPENGL_LIBRARIES} colormap)
target_link_libraries(cloudtest IMGUI ${GLFW3_LIBRARY} ${OPENGL_LIBRARIES} colormap)
set_target_properties(cloudlife PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set_target_properties(cloudtest PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(CTest)

add_test(NAME CloudTest1 COMMAND cloudtest -f 1024 -i 2 -r 512)
add_test(NAME CloudTest2 COMMAND cloudtest -f 32 -i 8 -r 16)
