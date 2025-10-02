#include "scene-loader.h"
#include "constants.h"
#include "obj-reader.h"
#include "bvh.h"

SceneData* loadScene()
{
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

    const int numTriangles = (int) triangles.size();
    std::vector<glm::vec4> triv0(numTriangles), triv1(numTriangles), triv2(numTriangles);
    for (int i = 0; i < numTriangles; i++) {
        triv0[i] = glm::vec4(triangleVertices[triangles[i].x], 0.0f);
        triv1[i] = glm::vec4(triangleVertices[triangles[i].y], 0.0f);
        triv2[i] = glm::vec4(triangleVertices[triangles[i].z], 0.0f);
    }
    free(contents);
    return new SceneData{ numTriangles, alignedBVHNodes, triv0, triv1, triv2 };
}