# Simple Find module for our generated Glad
set(GLAD_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/external/glad/include)
set(GLAD_SOURCES ${CMAKE_SOURCE_DIR}/external/glad/src/glad.c)

add_library(glad STATIC ${GLAD_SOURCES})
target_include_directories(glad PUBLIC ${GLAD_INCLUDE_DIRS})
