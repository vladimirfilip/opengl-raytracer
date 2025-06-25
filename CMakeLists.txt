cmake_minimum_required(VERSION 3.29)
project(opengl_raytracer)

set(CMAKE_CXX_STANDARD 20)

add_executable(opengl_raytracer main.cpp
        third_party/glad/src/glad.c)

include_directories(
        ${CMAKE_SOURCE_DIR}/third_party/glfw/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
    )
link_directories(${CMAKE_SOURCE_DIR}/third_party/glfw/lib-mingw-w64)

target_link_libraries(opengl_raytracer ${CMAKE_SOURCE_DIR}/third_party/glfw/lib-mingw-w64/libglfw3.a opengl32 gdi32 user32 kernel32)
