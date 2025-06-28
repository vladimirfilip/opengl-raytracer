cmake_minimum_required(VERSION 3.29)
project(opengl_raytracer)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

add_executable(opengl_raytracer main.cpp
        third_party/glad/src/glad.c)

FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)


include_directories(
        ${CMAKE_SOURCE_DIR}/third_party/glfw/include
        ${CMAKE_SOURCE_DIR}/third_party/glad/include
    )
link_directories(${CMAKE_SOURCE_DIR}/third_party/glfw/lib-mingw-w64)

target_link_libraries(opengl_raytracer glm::glm ${CMAKE_SOURCE_DIR}/third_party/glfw/lib-mingw-w64/libglfw3.a opengl32 gdi32 user32 kernel32)
