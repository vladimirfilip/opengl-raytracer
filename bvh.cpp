#include "bvh.h"

#include <iostream>
#include <vector>
#include <algorithm>

#include "constants.h"

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

#define NO_AXIS (-1)


static float getSA(glm::vec3 min, glm::vec3 max) {
    /*
     * Gets the surface area of a rectangular prism bounded by the two opposing vertices given
     */
    glm::vec3 dimensions = max - min;
    return 6 * (dimensions.x * dimensions.y + dimensions.x * dimensions.z +
                dimensions.y * dimensions.z);
}

float
getSplitCost(std::vector<glm::mat4x3> &triangleData, int start, int end, int axis, float splitVal) {
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
    return 1.0f + 50.0f * numLeft * getSA(leftMin, leftMax) +
           50.0f * numRight * getSA(rightMin, rightMax);
}

SplitInfo getSplit(BVHNode *node, std::vector<glm::mat4x3> &triangleData, int start, int end) {
    SplitInfo info = {NO_AXIS, 0.0f,
                      50.0f * (float) (end - start + 1) * getSA(node->minCorner, node->maxCorner)};
    if (node->isLeaf)
        return info;
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
    return info;
}

static int numLeaves = 0;
static int minLeafSize = (int) 1e9;
static int maxLeafSize = 0;
static int totalLeafSize = 0;
static int numNodesGenerated = 0;

static void initBVHGenerationStats() {
    numLeaves = maxLeafSize = totalLeafSize = numNodesGenerated = 0;
    minLeafSize = (int) 1e9;
}

static BVHNode* generateBVH(std::vector<glm::mat4x3> &triangleData, int start, int end, int nodeId = 0,
            int depth = 0) {
    /*
     * Recursively generates a BVH from a subarray of triangle data.
     * Chooses optimal split axis and split value according to SAH.
     * Swaps elements such that all nodes' triangles are grouped contiguously.
     */
    auto *node = new BVHNode();
    node->id = nodeId;
    for (int i = start; i <= end; i++) {
        node->addTriangle(triangleData[i]);
    }
    SplitInfo info = getSplit(node, triangleData, start, end);
    node->isLeaf = depth == MAX_BVH_DEPTH || info.axis == NO_AXIS;
    if (!node->isLeaf) {
        int numInLeftChild = 0;
        for (int i = start; i <= end; i++) {
            if (triangleData[i][0][info.axis] < info.splitVal) {
                std::swap(triangleData[start + numInLeftChild], triangleData[i]);
                numInLeftChild++;
            }
        }
        assert(0 < numInLeftChild && numInLeftChild <= end - start);
        int leftChildEnd = start + numInLeftChild - 1;
        int rightChildStart = leftChildEnd + 1;
        BVHNode *leftChild = generateBVH(triangleData, start, leftChildEnd, nodeId + 1, depth + 1);
        BVHNode *rightChild = generateBVH(triangleData, rightChildStart, end,
                                          nodeId + leftChild->treeSize + 1, depth + 1);
        node->children[0] = leftChild;
        node->children[1] = rightChild;
        node->treeSize += leftChild->treeSize + rightChild->treeSize;
    } else {
        node->triangleStart = start;
        node->triangleEnd = end;
        numLeaves++;
        int leafSize = end - start + 1;
        minLeafSize = std::min(minLeafSize, leafSize);
        maxLeafSize = std::max(maxLeafSize, leafSize);
        totalLeafSize += end - start + 1;
    }
    return node;
}

BVHNode *
generateBVH(std::vector<glm::uvec3> &triangleVertexIndices, std::vector<glm::vec3> &vertices) {
    /*
     * Generate BVH from a list of vertex coordinates
     * and the list of vertex indices for each triangle.
     */
    initBVHGenerationStats();
    /*
     * Converts the given triangle data into a 4x3 matrix with rows being:
     * triangle midpoint
     * triangle min point
     * triangle max point
     * the original vertex indices (converted to float + small padding to prevent floating point
     * errors)
     */
    std::vector<glm::mat4x3> triangleData(triangleVertexIndices.size());
    for (int i = 0; i < triangleVertexIndices.size(); i++) {
        glm::vec3 v1 = vertices[triangleVertexIndices[i][0]];
        glm::vec3 v2 = vertices[triangleVertexIndices[i][1]];
        glm::vec3 v3 = vertices[triangleVertexIndices[i][2]];
        triangleData[i][0] = (v1 + v2 + v3) / 3.0f;
        triangleData[i][1] = min(v1, min(v2, v3));
        triangleData[i][2] = max(v1, max(v2, v3));
        triangleData[i][3] = (glm::vec3) triangleVertexIndices[i] + glm::vec3(0.2f, 0.2f, 0.2f);
    }
    BVHNode *res = generateBVH(triangleData, 0, (int) triangleData.size() - 1);
    for (int i = 0; i < triangleData.size(); i++) {
        triangleVertexIndices[i] = triangleData[i][3];
    }
    numNodesGenerated = res->treeSize;
    std::cout << "GENERATED " << numNodesGenerated << " BVH NODES" << std::endl;
    std::cout << "NUM LEAVES: " << numLeaves << std::endl;
    std::cout << "MIN LEAF SIZE: " << minLeafSize << std::endl;
    std::cout << "MAX LEAF SIZE: " << maxLeafSize << std::endl;
    std::cout << "NUM TRIANGLES: " << totalLeafSize << std::endl;
    std::cout << "AVERAGE LEAF SIZE: " << (float) totalLeafSize / (float) numLeaves << std::endl;
    return res;
}

static void serialiseBVHNode(std::vector<glm::mat3> &v, int &i, BVHNode *node) {
    assert(i == node->id);
    glm::vec3 childData;
    if (node->isLeaf) {
        childData.x = 1.0f;
        childData.y = (float) node->triangleStart;
        childData.z = (float) node->triangleEnd;
    } else {
        childData.x = 0.0f;
        childData.y = (float) node->children[0]->id;
        childData.z = (float) node->children[1]->id;
    }
    v[i++] = glm::mat3(node->minCorner, node->maxCorner, childData);
    if (!node->isLeaf) {
        serialiseBVHNode(v, i, node->children[0]);
        serialiseBVHNode(v, i, node->children[1]);
    }
}

std::vector<glm::mat3> serialiseBVH(BVHNode *root) {
    std::vector<glm::mat3> v(numNodesGenerated);
    int i = 0;
    serialiseBVHNode(v, i, root);
    assert(i == numNodesGenerated);
    return v;
}

void freeBVH(BVHNode *node) {
    if (!node->isLeaf) {
        freeBVH(node->children[0]);
        freeBVH(node->children[1]);
    }
    free(node);
}
