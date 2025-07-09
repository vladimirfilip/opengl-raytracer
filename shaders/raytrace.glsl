#version 430

layout(local_size_x = 32, local_size_y = 32) in;

#define WHITE vec4(1.0f, 1.0f, 1.0f, 1.0f)
#define BLACK vec4(0.0f, 0.0f, 0.0f, 1.0f)
#define INFINITY 1.0 / 0.0
#define MAX_BVH_TRAVERSAL_STACK_SIZE 100

uniform float u_ScreenWidth;
uniform float u_ScreenHeight;
uniform float u_FOV;
uniform float u_ViewportDist;
uniform uint u_RayBounces;
uniform uint u_RaysPerPixel;
uniform vec3 cameraPos;
uniform mat3 cameraRotation;

layout(std430, binding = 0) buffer VertexBuffer {
    vec3 vertices[];
};

layout(std430, binding = 1) buffer TriangleBuffer {
    uvec3 triangles[];
};

layout(std430, binding = 2) buffer NormalsBuffer {
    vec3 triangleNormals[];
};

layout(binding = 3, rgba32f) uniform image2D pixelOutput;

layout(std430, binding = 4) buffer BVHBuffer {
    mat3 bvh[];
};

struct HitInfo {
    float dist;
    int triangleIndex;
};

const float EPS = 1e-3;

/*
 * Gets distance the ray has to travel before hitting the triangle
 * distance = INFINITY if ray does not intersect
 */
float getRayTriangleDistance(vec3 origin, vec3 dir, int triangleIndex) {
    vec3 a = vertices[triangles[triangleIndex].x];
    vec3 b = vertices[triangles[triangleIndex].y];
    vec3 c = vertices[triangles[triangleIndex].z];
    vec3 n = triangleNormals[triangleIndex];
    float denom = dot(dir, n);
    if (abs(denom) < EPS)
        return INFINITY;
    float alpha = dot(a - origin, n) / denom;
    if (alpha <= EPS)
        return INFINITY;
    // x = point on the ray that intersects the plane of the triangle
    vec3 x = origin + alpha * dir;
    // x is inside the triangle if sum of areas of triangles formed by x and each pair of triangle vertices equals
    // the area of the triangle
    float area = length(cross(x - a, x - b)) + length(cross(x - b, x - c)) + length(cross(x - c, x - a));
    if (abs(area - length(n)) >= EPS)
        return INFINITY;
    return alpha;
}

float getRayXYPlaneDistance(vec3 origin, vec3 dir, vec4 points, float z) {
    float alpha = (z - origin.z) / dir.z;
    if (alpha <= EPS)
        return INFINITY;
    vec3 a = origin + alpha * dir;
    if (points.x - a.x > EPS || a.x - points.z > EPS || points.y - a.y > EPS || a.y - points.w > EPS)
        return INFINITY;
    return alpha;
}
float getRayXZPlaneDistance(vec3 origin, vec3 dir, vec4 points, float y) {
    float alpha = (y - origin.y) / dir.y;
    if (alpha <= EPS)
        return INFINITY;
    vec3 a = origin + alpha * dir;
    if (points.x - a.x > EPS || a.x - points.z > EPS || points.y - a.z > EPS || a.z - points.w > EPS)
        return INFINITY;
    return alpha;
}
float getRayYZPlaneDistance(vec3 origin, vec3 dir, vec4 points, float x) {
    float alpha = (x - origin.x) / dir.x;
    if (alpha <= EPS)
        return INFINITY;
    vec3 a = origin + alpha * dir;
    if (points.x - a.y > EPS || a.y - points.z > EPS || points.y - a.z > EPS || a.z - points.w > EPS)
        return INFINITY;
    return alpha;
}

// Thanks to https://gist.github.com/DomNomNom/46bb1ce47f68d255fd5d
bool rayIntersectsBoundingBox(vec3 origin, vec3 dir, vec3 boxMin, vec3 boxMax) {
    vec3 invDir = 1 / dir;
    vec3 tMin = (boxMin - origin) * invDir;
    vec3 tMax = (boxMax - origin) * invDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return tNear <= tFar;
}

HitInfo getHitInfo(vec3 origin, vec3 dir) {
    HitInfo info;
    info.dist = INFINITY;
    info.triangleIndex = -1;
/* Old code, loop over all triangles
    for (int i = 0; i < triangles.length(); i++) {
        float triangleDist = getRayTriangleDistance(origin, dir, i);
        if (triangleDist < info.dist) {
            info.dist = triangleDist;
            info.triangleIndex = i;
        }
    }
*/
    int stack[MAX_BVH_TRAVERSAL_STACK_SIZE];
    int i = 0;
    stack[0] = 0;
    while (i > -1) {
        int bvhIndex = stack[i];
        if (!rayIntersectsBoundingBox(origin, dir, bvh[bvhIndex][0], bvh[bvhIndex][1])) {
            i--;
            continue;
        }
        bool isLeaf = abs(bvh[bvhIndex][2].x - 1.0f) <= EPS;
        if (isLeaf) {
            int triangleStart = int(bvh[bvhIndex][2].y);
            int triangleEnd = int(bvh[bvhIndex][2].z);
            for (int i = triangleStart; i <= triangleEnd; i++) {
                float triangleDist = getRayTriangleDistance(origin, dir, i);
                if (triangleDist < info.dist) {
                    info.dist = triangleDist;
                    info.triangleIndex = i;
                }
            }
        } else {
            int child1 = int(bvh[bvhIndex][2].y);
            int child2 = int(bvh[bvhIndex][2].z);
            float d1 = length((bvh[child1][0] + bvh[child1][1]) / 2.0 - origin);
            float d2 = length((bvh[child2][0] + bvh[child2][1]) / 2.0 - origin);
            if (d1 < d2) {
                stack[i++] = child1;
                stack[i++] = child2;
            } else {
                stack[i++] = child2;
                stack[i++] = child1;
            }
        }
        i--;
    }
    return info;
}

vec4 getColour(vec3 origin, vec3 dir, uint bouncesLeft) {
    HitInfo info = getHitInfo(origin, dir);
    return info.dist < INFINITY ? WHITE : BLACK;
}

void main() {
    if (gl_GlobalInvocationID.x >= u_ScreenWidth || gl_GlobalInvocationID.y >= u_ScreenHeight) {
        return;
    }
    // tan (FOV / 2) = (viewportWidth / 2) / viewportDist
    float pixelWidth = tan(u_FOV / 2.0f) * u_ViewportDist * 2.0f / u_ScreenWidth;
    vec3 dir = normalize(vec3(
        (float(gl_GlobalInvocationID.x) - u_ScreenWidth / 2.0f) * pixelWidth,
        (float(gl_GlobalInvocationID.y) - u_ScreenHeight / 2.0f) * pixelWidth,
        u_ViewportDist
    ));
    vec3 rotatedDir = cameraRotation * dir;
    vec3 rayOrigin = cameraPos + rotatedDir;
    vec4 colour = BLACK;
    for (uint i = uint(0); i < u_RaysPerPixel; i++) {
        colour += getColour(rayOrigin, rotatedDir, u_RayBounces);
    }
    colour /= u_RaysPerPixel;
    imageStore(pixelOutput, ivec2(gl_GlobalInvocationID.xy), colour);
}