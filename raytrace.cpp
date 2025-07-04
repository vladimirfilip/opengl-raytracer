#include "raytrace.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <numbers>
#include <iostream>
#include <vector>

#include "constants.h"
#include "util.h"
#include "obj-reader.h"

#define WORKGROUP_SIZE 32

static GLuint raytraceProgram;
static GLuint groups_x, groups_y;
static int screenWidth, screenHeight;

template<typename T>
void initSSBO(std::vector<T> &data, unsigned int binding) {
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glGenBuffers");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBindBuffer");
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(T), data.data(), GL_DYNAMIC_DRAW);
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
    ObjContents *contents = readObjContents("../model.obj");
    std::vector<glm::vec4> triangleVertices = contents->vertices;
    initSSBO(triangleVertices, VERTEX_SSBO_BINDING);
    std::vector<glm::uvec4> triangles = contents->triangles;
    initSSBO(triangles, TRIANGLE_SSBO_BINDING);
    free(contents);
    std::vector<glm::vec4> triangleNormals = std::vector<glm::vec4>(triangles.size());
    initSSBO(triangleNormals, TRIANGLE_NORMAL_SSBO_BINDING);
    GLuint normalShader = importAndCompileShader("../shaders/normals.glsl", GL_COMPUTE_SHADER);
    runComputeShader(normalShader, (int) triangles.size(), 1, 1);
    // Static constants cast to float
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    raytraceProgram = generateProgram(
            importAndCompileShader("../shaders/raytrace.glsl", GL_COMPUTE_SHADER));
    glUseProgram(raytraceProgram);
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ScreenWidth"),
                static_cast<GLfloat>(screenWidth));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ScreenHeight"),
                static_cast<GLfloat>(screenHeight));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_FOV"),
                static_cast<GLfloat>(FOV * std::numbers::pi / 180.f));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ViewportDist"), VIEWPORT_DIST);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RaysPerPixel"), RAYS_PER_PIXEL);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RayBounces"), RAY_BOUNCES);
    groups_x = (screenWidth + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
    groups_y = (screenHeight + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
};

void raytrace(glm::vec3 cameraPos, glm::mat3 cameraRotation) {
    glUseProgram(raytraceProgram);
    glUniform3f(glGetUniformLocation(raytraceProgram, "cameraPos"), cameraPos.x, cameraPos.y,
                cameraPos.z);
    glUniformMatrix3fv(glGetUniformLocation(raytraceProgram, "cameraRotation"), 1, GL_FALSE, glm::value_ptr(cameraRotation));
    glDispatchCompute(groups_x, groups_y, 1);
}