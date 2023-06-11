cmake_minimum_required(VERSION 3.16.3)

project(imgui-cmake LANGUAGES C CXX)


set (CMAKE_C_FLAGS_RELEASE "-O3 -march=native")
set (CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
set (CMAKE_C_FLAGS_DEBUG "-O0 -ggdb3 -fsanitize=address")
set (CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})

set (CMAKE_VERBOSE_MAKEFILE TRUE)


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

find_package(glfw3)

add_subdirectory(colormap)

add_executable(cloudlife)
target_sources(cloudlife PUBLIC
    main.cpp
    artfactory.cpp
    settings.cpp
    imgui_elements.cpp
    random.c
    cloudlife.cpp
    mtron.cpp
    timer.cpp
    ifs.cpp
    vermiculate.cpp
    discrete.cpp
    thornbird.cpp)
target_link_libraries(cloudlife IMGUI glfw GL colormap)
set_target_properties(cloudlife PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
