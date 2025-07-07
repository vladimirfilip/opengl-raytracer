#include "bvh.h"

#include <iostream>
#include <stack>
#include <vector>
#include <algorithm>
#include <functional>

#include "constants.h"

static int n = 0;

enum Axis {X, Y, Z};

struct SplitInfo {
    int splitIndex;
    Axis axis;
    float minCost;
};

SplitInfo min(SplitInfo a, SplitInfo b) {
    if (a.minCost < b.minCost)
        return a;
    return b;
}


static void addTriangle(BVHNode* node, glm::mat4x3& triangleData) {
    node->minCorner = min(node->minCorner, triangleData[1]);
    node->maxCorner = max(node->maxCorner, triangleData[2]);
}

static float getSA(glm::vec3 dimensions) {
    return 6 * (dimensions.x * dimensions.y +
        dimensions.x * dimensions.z +
        dimensions.y * dimensions.z);
}

static auto sortByX = [](const glm::mat4x3& a, const glm::mat4x3& b) {
    return a[0].x < b[0].x;
};

static auto sortByY = [](const glm::mat4x3& a, const glm::mat4x3& b) {
    return a[0].y < b[0].y;
};

static auto sortByZ = [](const glm::mat4x3& a, const glm::mat4x3& b) {
    return a[0].z < b[0].z;
};

SplitInfo splitOnAxis(std::vector<glm::mat4x3>& triangleData,
                 int start, int end, Axis axis) {
    SplitInfo res{-1, axis, INFINITY};
    std::vector<glm::vec3> minVertexPrefix(end - start + 1),
            maxVertexPrefix(end - start + 1),
            minVertexSuffix(end - start + 1),
            maxVertexSuffix(end - start + 1);
    minVertexPrefix[0] = triangleData[start][1];
    maxVertexPrefix[0] = triangleData[start][2];
    for (int i = 1; i < minVertexPrefix.size(); i++) {
        minVertexPrefix[i] = min(minVertexPrefix[i - 1], triangleData[start + i][1]);
        maxVertexPrefix[i] = max(maxVertexPrefix[i - 1], triangleData[start + i][2]);
    }
    minVertexSuffix[end - start] = triangleData[end][1];
    maxVertexSuffix[end - start] = triangleData[end][2];
    for (int i = end - start - 1; i >= 0; i--) {
        minVertexSuffix[i] = min(minVertexSuffix[i + 1], triangleData[start + i][1]);
        maxVertexSuffix[i] = max(maxVertexSuffix[i + 1], triangleData[start + i][2]);
    }
    for (int i = 0; i < minVertexPrefix.size() - 1; i++) {
        glm::vec3 leftDimensions = maxVertexPrefix[i] - minVertexPrefix[i];
        glm::vec3 rightDimensions = maxVertexSuffix[i + 1] - minVertexSuffix[i + 1];
        float leftSA = getSA(leftDimensions);
        float rightSA = getSA(rightDimensions);
        float cost = leftSA + rightSA;
        if (cost < res.minCost) {
            res.splitIndex = i;
            res.minCost = cost;
        }
    }
    assert(res.splitIndex != -1);
    return res;
}

SplitInfo getSplit(std::vector<glm::mat4x3>& triangleData, int start, int end) {
    std::sort(triangleData.begin() + start, triangleData.begin() + end + 1, sortByX);
    SplitInfo info = splitOnAxis(triangleData, start, end, X);
    std::sort(triangleData.begin() + start, triangleData.begin() + end + 1, sortByY);
    info = min(info, splitOnAxis(triangleData, start, end, Y));
    std::sort(triangleData.begin() + start, triangleData.begin() + end + 1, sortByZ);
    info = min(info, splitOnAxis(triangleData, start, end, Z));
    return info;
}

static BVHNode* generateBVH(std::vector<glm::mat4x3>& triangleData) {
    std::stack<std::pair<int, int>> intervals;
    intervals.emplace(0, triangleData.size() - 1);
    std::unordered_map<int, BVHNode*> startToNode, endToNode;
    BVHNode *res = nullptr;
    while (!intervals.empty()) {
        std::cout << intervals.size() << std::endl;
        auto [start, end] = intervals.top();
        intervals.pop();
        auto* node = new BVHNode();
        if (res == nullptr)
            res = node;
        node->id = n++;
        if (end - start + 1 <= MAXIMUM_BVH_LEAF_TRIANGLE_COUNT) {
            node->isLeaf = true;
            node->triangleStart = start;
            node->triangleEnd = end;
        }
        for (int i = start; i <= end; i++) {
            addTriangle(node, triangleData[i]);
        }
        if (startToNode.count(start))
            startToNode[start]->children[0] = node;
        if (endToNode.count(end))
            endToNode[end]->children[1] = node;
        if (!node->isLeaf) {
            startToNode[start] = node;
            endToNode[end] = node;
            SplitInfo info = getSplit(triangleData, start, end);
            std::function<bool(glm::mat4, glm::mat4)> comp;
            switch (info.axis) {
                case X:
                    comp = sortByX;
                    break;
                case Y:
                    comp = sortByY;
                    break;
                case Z:
                    comp = sortByZ;
                    break;
            }
            std::sort(triangleData.begin() + start, triangleData.begin() + end + 1, comp);
            intervals.emplace(info.splitIndex + 1, end);
            intervals.emplace(start, info.splitIndex);
        }
    }
    return res;
}

BVHNode* generateBVH(std::vector<glm::uvec3>& triangles, std::vector<glm::vec3>& vertices) {
    std::vector<glm::mat4x3> triangleData(triangles.size());
    for (int i = 0; i < triangles.size(); i++) {
        glm::vec3 v1 = vertices[triangles[i][0]];
        glm::vec3 v2 = vertices[triangles[i][1]];
        glm::vec3 v3 = vertices[triangles[i][2]];
        triangleData[i][0] = (v1 + v2 + v3) / 3.0f;
        triangleData[i][1] = min(v1, min(v2, v3));
        triangleData[i][2] = max(v1, max(v2, v3));
        triangleData[i][3] = triangles[i];
    }
    BVHNode* res = generateBVH(triangleData);
    for (int i = 0; i < triangleData.size(); i++) {
        triangles[i] = triangleData[i][3];
    }
    std::cout << "GENERATED " << n << " BVH NODES" << std::endl;
    return res;
}

static void serialiseBVHNode(std::vector<glm::mat3>& v, int& i, BVHNode *node) {
    assert(i == node->id);
    glm::vec3 childData;
    if (node->isLeaf) {
        childData.x = 1.0f;
        childData.y = node->triangleStart;
        childData.z = node->triangleEnd;
    } else {
        childData.x = 0.0f;
        childData.y = node->children[0]->id;
        childData.z = node->children[1]->id;
    }
    v[i++] = glm::mat3(node->minCorner, node->maxCorner, childData);
    if (!node->isLeaf) {
        serialiseBVHNode(v, i, node->children[0]);
        serialiseBVHNode(v, i, node->children[1]);
    }
}

std::vector<glm::mat3> serialiseBVH(BVHNode *root) {
    std::vector<glm::mat3> v(n);
    int i = 0;
    serialiseBVHNode(v, i, root);
    assert(i == n);
    return v;
}

void freeBVH(BVHNode *node) {
    if (!node->isLeaf) {
        freeBVH(node->children[0]);
        freeBVH(node->children[1]);
    }
    free(node);
}
