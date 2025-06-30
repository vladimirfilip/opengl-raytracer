#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <numbers>
#include <iostream>
#include <vector>

#include "constants.h"
#include "util.h"

static GLuint raytraceProgram;
static std::vector<glm::vec4> triangleVertices, triangleNormals;
static std::vector<glm::uvec4> triangles;
static int screenWidth, screenHeight;
static GLuint groups_x, groups_y;

template <typename T>
void initSSBO(std::vector<T>& data, unsigned int binding) {
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glGenBuffers");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBindBuffer");
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 data.size() * sizeof(T),
                 data.data(),
                 GL_DYNAMIC_DRAW);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBufferData");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBindBufferBase");
}

void runComputeShader(GLuint shader, int num_groups_x, int num_groups_y, int num_groups_z) {
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glUseProgram(program);
    glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    glDeleteProgram(program);
}


void raytraceInit() {
    // Building internal triangle and vertex buffers
    triangleVertices = {
            {-1.0f, 1.0f, 10.0f, 1.0f},
            {1.0f, 1.0f, 10.0f, 1.0f},
            {0.0f, -1.0f, 10.0f, 1.0f},
            {3.0f, 1.0f, 10.0f, 1.0f},
            {5.0f, 1.0f, 7.0f, 1.0f},
            {4.0f, -1.0f, 8.0f, 1.0f},
            {-3.0f, 1.0f, 10.0f, 1.0f},
            {-5.0f, 1.0f, 7.0f, 1.0f},
            {-4.0f, -1.0f, 8.0f, 1.0f},
    };
    initSSBO(triangleVertices, VERTEX_SSBO_BINDING);
    triangles = {
            {0, 1, 2, 0},
            {3, 4, 5, 0},
            {6, 7, 8, 0},
    };
    initSSBO(triangles, TRIANGLE_SSBO_BINDING);
    triangleNormals = std::vector<glm::vec4>(triangles.size());
    initSSBO(triangleNormals, TRIANGLE_NORMAL_SSBO_BINDING);
    GLuint normalShader = importAndCompileShader("../normals.glsl", GL_COMPUTE_SHADER);
    runComputeShader(normalShader, (int) triangles.size(), 1, 1);
    // Static constants cast to float
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    raytraceProgram = generateProgram(importAndCompileShader("../raytrace.glsl", GL_COMPUTE_SHADER));
    glUseProgram(raytraceProgram);
    glUniform1f(
            glGetUniformLocation(raytraceProgram, "u_ScreenWidth"),
            static_cast<GLfloat>(screenWidth));
    glUniform1f(
            glGetUniformLocation(raytraceProgram, "u_ScreenHeight"),
            static_cast<GLfloat>(screenHeight));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_FOV"),
                static_cast<GLfloat>(FOV * std::numbers::pi / 180.f));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ViewportDist"),
                VIEWPORT_DIST);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RaysPerPixel"), RAYS_PER_PIXEL);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RayBounces"), RAY_BOUNCES);
    groups_x = (screenWidth + 16 - 1) / 16;
    groups_y = (screenHeight + 16 - 1) / 16;
};

void raytrace() {
    glUseProgram(raytraceProgram);
    glDispatchCompute(groups_x, groups_y, 1);
}