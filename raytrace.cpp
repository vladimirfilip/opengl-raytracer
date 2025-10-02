#include "raytrace.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <random>

#include "constants.h"
#include "util.h"
#include "obj-reader.h"

static GLuint raytraceProgram;

template<typename T>
void initSSBO(std::vector<T> &data, unsigned int binding) {
    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glGenBuffers");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBindBuffer");
    glBufferData(GL_SHADER_STORAGE_BUFFER, data.size() * sizeof(T), data.data(), GL_STATIC_DRAW);
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

void initBuffers(SceneData* sceneData) {
    initSSBO(sceneData->alignedBVHNodes, BVH_BINDING);
    initSSBO(sceneData->triv0, TRI_V0_SSBO_BINDING);
    initSSBO(sceneData->triv1, TRI_V1_SSBO_BINDING);
    initSSBO(sceneData->triv2, TRI_V2_SSBO_BINDING);
    int numTriangles = sceneData->numTriangles;
    std::vector<glm::vec4> triangleColours(numTriangles);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int i = 0; i < numTriangles; i++) {
        triangleColours[i] = glm::vec4(dist(gen)
                , dist(gen), dist(gen), 1.0f);
    }
    initSSBO(triangleColours, TRIANGLE_COLOUR_SSBO_BINDING);
    std::vector<glm::vec4> triangleNormals = std::vector<glm::vec4>(numTriangles);
    initSSBO(triangleNormals, TRIANGLE_NORMAL_SSBO_BINDING);
    GLuint normalShader = importAndCompileShader("../shaders/normals.glsl", GL_COMPUTE_SHADER);
    runComputeShader(normalShader, numTriangles, 1, 1);
}

GLuint raytraceInit(SceneData* sceneData, int screenWidth, int screenHeight) {
    GLuint raytraceShader = importAndCompileShader("../shaders/raytrace.glsl", GL_COMPUTE_SHADER);
    raytraceProgram = generateProgram(raytraceShader);
    glUseProgram(raytraceProgram);
    initBuffers(sceneData);
    checkGLError("(raytraceInit) initBuffers()");
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ScreenWidth"),
                static_cast<GLfloat>(screenWidth));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ScreenHeight"),
                static_cast<GLfloat>(screenHeight));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_FOV"),
                static_cast<GLfloat>(FOV * std::numbers::pi / 180.f));
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ViewportDist"), VIEWPORT_DIST);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RaysPerPixel"), RAYS_PER_PIXEL);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RayBounces"), RAY_BOUNCES);
    checkGLError("(raytraceInit) set uniforms");
    return raytraceProgram;
};

void raytrace(glm::vec3 cameraPos, glm::mat3 cameraRotation, int renderMode, int frameCount, int num_groups_x, int num_groups_y) {
    glUseProgram(raytraceProgram);
    glUniform3f(glGetUniformLocation(raytraceProgram, "cameraPos"), cameraPos.x, cameraPos.y,
                cameraPos.z);
    glUniformMatrix3fv(glGetUniformLocation(raytraceProgram, "cameraRotation"), 1, GL_FALSE,
                       glm::value_ptr(cameraRotation));
    glUniform1ui(glGetUniformLocation(raytraceProgram, "renderMode"), renderMode);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "frameCount"), frameCount);
    glDispatchCompute(num_groups_x, num_groups_y, 1);
}
