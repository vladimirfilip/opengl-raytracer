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
#include "bvh.h"

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
    std::vector<glm::vec4> triangleNormals = std::vector<glm::vec4>(triangles.size());
    initSSBO(triangleNormals, TRIANGLE_NORMAL_SSBO_BINDING);
    GLuint normalShader = importAndCompileShader("../shaders/normals.glsl", GL_COMPUTE_SHADER);
    runComputeShader(normalShader, (int) triangles.size(), 1, 1);
}

void raytraceInit(GLuint program) {
    initBuffers();
    checkGLError("(raytraceInit) initBuffers()");
};
