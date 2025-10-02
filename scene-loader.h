#ifndef OPENGL_RAYTRACER_SCENE_LOADER_H
#define OPENGL_RAYTRACER_SCENE_LOADER_H

#include <glm/glm.hpp>
#include <vector>

struct AlignedMat3 {
    glm::vec4 u, v, w;
};

struct SceneData {
    int numTriangles;
    std::vector<AlignedMat3> alignedBVHNodes;
    std::vector<glm::vec4> triv0, triv1, triv2;
};

extern SceneData* loadScene();

#endif //OPENGL_RAYTRACER_SCENE_LOADER_H