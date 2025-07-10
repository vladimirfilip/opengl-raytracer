#ifndef OPENGL_RAYTRACER_BVH_H
#define OPENGL_RAYTRACER_BVH_H

#include "glm/glm.hpp"

#include <vector>
#include <cmath>

#define MIN_VERTEX glm::vec3{-1e9, -1e9, -1e9}
#define MAX_VERTEX glm::vec3{1e9, 1e9, 1e9}

struct BVHNode {
    int id;
    glm::vec3 minCorner = MAX_VERTEX;
    glm::vec3 maxCorner = MIN_VERTEX;
    BVHNode* children[2];
    int triangleStart, triangleEnd;
    bool isLeaf = false;
};

extern BVHNode* generateBVH(std::vector<glm::uvec3>& triangles, std::vector<glm::vec3>& vertices);

extern std::vector<glm::mat3> serialiseBVH(BVHNode* node);

extern void freeBVH(BVHNode* node);

#endif //OPENGL_RAYTRACER_BVH_H
