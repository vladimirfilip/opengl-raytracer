#include "raytrace.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <random>

#include "constants.h"
#include "util.h"
#include "obj-reader.h"
#include "bvh.h"

static GLuint raytraceProgram;

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

struct AlignedMat3 {
    glm::vec4 u, v, w;
};

void initBuffers() {
    ObjContents *contents = readObjContents(SCENE_FILE_PATH);
    std::vector<glm::vec3> triangleVertices = contents->vertices;
    std::vector<glm::uvec3> triangles = contents->triangles;
    auto bvh = generateBVH(triangles, triangleVertices);
    auto bvhNodes = serialiseBVH(bvh);
    std::vector<AlignedMat3> alignedBVHNodes(bvhNodes.size());
    for (int i = 0; i < bvhNodes.size(); i++) {
        alignedBVHNodes[i] = AlignedMat3{
            glm::vec4(bvhNodes[i][0], 0.0f),
            glm::vec4(bvhNodes[i][1], 0.0f),
        glm::vec4(bvhNodes[i][2], 0.0f) };
    }
    initSSBO(alignedBVHNodes, BVH_BINDING);
    std::vector<AlignedMat3> alignedTriangles(triangles.size());
    for (int i = 0; i < alignedTriangles.size(); i++) {
        alignedTriangles[i] = AlignedMat3{
            glm::vec4(triangleVertices[triangles[i].x], 0.0f),
            glm::vec4(triangleVertices[triangles[i].y], 0.0f),
            glm::vec4(triangleVertices[triangles[i].z], 0.0f)
        };
    }
    initSSBO(alignedTriangles, TRIANGLE_SSBO_BINDING);
    free(contents);
    std::vector<glm::vec4> triangleColours(triangles.size());
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (int i = 0; i < triangles.size(); i++) {
        triangleColours[i] = glm::vec4(dist(gen)
                , dist(gen), dist(gen), 1.0f);
    }
    initSSBO(triangleColours, TRIANGLE_COLOUR_SSBO_BINDING);
    std::vector<glm::vec4> triangleNormals = std::vector<glm::vec4>(triangles.size());
    initSSBO(triangleNormals, TRIANGLE_NORMAL_SSBO_BINDING);
    GLuint normalShader = importAndCompileShader("../shaders/normals.glsl", GL_COMPUTE_SHADER);
    runComputeShader(normalShader, (int) triangles.size(), 1, 1);
}

GLuint raytraceInit() {
    GLuint raytraceShader = importAndCompileShader("../shaders/raytrace.glsl", GL_COMPUTE_SHADER);
    raytraceProgram = generateProgram(raytraceShader);
    glUseProgram(raytraceProgram);
    initBuffers();
    checkGLError("(raytraceInit) initBuffers()");
    return raytraceProgram;
};

void raytrace(glm::vec3 cameraPos, glm::mat3 cameraRotation, int renderMode, int num_groups_x, int num_groups_y) {
    glUseProgram(raytraceProgram);
    glUniform3f(glGetUniformLocation(raytraceProgram, "cameraPos"), cameraPos.x, cameraPos.y,
                cameraPos.z);
    glUniformMatrix3fv(glGetUniformLocation(raytraceProgram, "cameraRotation"), 1, GL_FALSE,
                       glm::value_ptr(cameraRotation));
    glUniform1ui(glGetUniformLocation(raytraceProgram, "renderMode"), renderMode);
    glDispatchCompute(num_groups_x, num_groups_y, 1);
}
