#include "bvh.h"

#include <iostream>
#include <stack>
#include <vector>
#include <algorithm>
#include <functional>

#include "constants.h"

static int n = 0;

struct SplitInfo {
    int axis;
    float splitVal;
    float cost;
};

SplitInfo min(SplitInfo a, SplitInfo b) {
    if (a.cost < b.cost)
        return a;
    return b;
}


static void addTriangle(BVHNode* node, glm::mat4x3& triangleData) {
    node->minCorner = min(node->minCorner, triangleData[1]);
    node->maxCorner = max(node->maxCorner, triangleData[2]);
}

static float getSA(glm::vec3 min, glm::vec3 max) {
    glm::vec3 dimensions = max - min;
    return 6 * (dimensions.x * dimensions.y +
        dimensions.x * dimensions.z +
        dimensions.y * dimensions.z);
}

float getSplitCost(std::vector<glm::mat4x3>& triangleData, int start, int end, int axis, float splitVal) {
    glm::vec3 leftMin = MAX_VERTEX, rightMin = MAX_VERTEX, leftMax = MIN_VERTEX, rightMax = MIN_VERTEX;
    float numLeft = 0, numRight = 0;
    for (int j = start; j <= end; j++) {
        if (triangleData[j][0][axis] < splitVal) {
            leftMin = min(leftMin, triangleData[j][1]);
            leftMax = max(leftMax, triangleData[j][2]);
            numLeft++;
        } else {
            rightMin = min(rightMin, triangleData[j][1]);
            rightMax = max(rightMax, triangleData[j][2]);
            numRight++;
        }
    }
    leftMax = max(leftMax, leftMin);
    rightMax = max(rightMax, rightMin);
    return 1.0f + 50.0f * numLeft * getSA(leftMin, leftMax) + 50.0f * numRight * getSA(rightMin, rightMax);
}

SplitInfo getSplit(BVHNode* node, std::vector<glm::mat4x3>& triangleData, int start, int end) {
    SplitInfo info{-1, 0.0f, 50.0f * (float) (end - start + 1) * getSA(node->minCorner, node->maxCorner)};
    for (int axis = 0; axis < 3; axis++) {
        float minVal = node->minCorner[axis];
        float maxVal = node->maxCorner[axis];
        float minDiff = 1e-6;
        for (int i = 0; i < BVH_SPLIT_ITERATIONS && abs(minVal - maxVal) > minDiff; i++) {
            float third = (maxVal - minVal) / 3.0f;
            float val1 = minVal + third, val2 = maxVal - third;
            float cost1 = getSplitCost(triangleData, start, end, axis, val1);
            float cost2 = getSplitCost(triangleData, start, end, axis, val2);
            if (cost1 <= cost2) {
                maxVal = val2;
            }
            if (cost2 <= cost1) {
                minVal = val1;
            }
        }
        float splitVal = (minVal + maxVal) / 2.0f;
        float cost = getSplitCost(triangleData, start, end, axis, splitVal);
        info = min(info, {axis, splitVal, cost});
    }
    assert(info.cost < INFINITY);
    return info;
}

static BVHNode* generateBVH(std::vector<glm::mat4x3>& triangleData, int start, int end, int depth = 0) {
    auto* res = new BVHNode();
    res->id = n++;
    res->isLeaf = depth == MAX_BVH_DEPTH;
    for (int i = start; i <= end; i++) {
        addTriangle(res, triangleData[i]);
    }
    SplitInfo info;
    if (!res->isLeaf) {
        info = getSplit(res, triangleData, start, end);
        if (info.axis == -1)
            res->isLeaf = true;
    }
    if (!res->isLeaf) {
        int numOnLeft = 0;
        for (int i = start; i <= end; i++) {
            if (triangleData[i][0][info.axis] < info.splitVal) {
                std::swap(triangleData[start + numOnLeft], triangleData[i]);
                numOnLeft++;
            }
        }
        assert(0 < numOnLeft && numOnLeft <= end - start);
        res->children[0] = generateBVH(triangleData, start, start + numOnLeft - 1, depth + 1);
        res->children[1] = generateBVH(triangleData, start + numOnLeft, end, depth + 1);
    } else {
        res->triangleStart = start;
        res->triangleEnd = end;
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
        triangleData[i][3] = (glm::vec3) triangles[i] + glm::vec3(0.2f, 0.2f, 0.2f);
    }
    BVHNode* res = generateBVH(triangleData, 0, triangleData.size() - 1);
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
