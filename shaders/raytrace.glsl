#version 430

layout(local_size_x = 32, local_size_y = 32) in;

#define WHITE vec4(1.0f, 1.0f, 1.0f, 1.0f)
#define BLACK vec4(0.0f, 0.0f, 0.0f, 1.0f)
#define INFINITY 1.0 / 0.0
#define MAX_BVH_TRAVERSAL_STACK_SIZE 1024

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

struct Ray {
    vec3 origin, dir, invDir;
};

const float EPS = 1e-3;

/*
 * Gets distance the ray has to travel before hitting the triangle
 * distance = INFINITY if ray does not intersect
 */
float getRayTriangleDistance(Ray ray, int triangleIndex) {
    vec3 a = vertices[triangles[triangleIndex].x];
    vec3 b = vertices[triangles[triangleIndex].y];
    vec3 c = vertices[triangles[triangleIndex].z];
    vec3 n = triangleNormals[triangleIndex];
    float denom = dot(ray.dir, n);
    if (abs(denom) < EPS)
        return INFINITY;
    float alpha = dot(a - ray.origin, n) / denom;
    if (alpha <= EPS)
        return INFINITY;
    // x = point on the ray that intersects the plane of the triangle
    vec3 x = ray.origin + alpha * ray.dir;
    // x is inside the triangle if sum of areas of triangles formed by x and each pair of triangle vertices equals
    // the area of the triangle
    float area = length(cross(x - a, x - b)) + length(cross(x - b, x - c)) + length(cross(x - c, x - a));
    if (abs(area - length(n)) >= EPS)
        return INFINITY;
    return alpha;
}

// Thanks to https://tavianator.com/2011/ray_box.html
float rayBoundingBoxDist(Ray ray, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - ray.origin) * ray.invDir;
    vec3 tMax = (boxMax - ray.origin) * ray.invDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);

    bool hit = tFar >= tNear && tFar > 0;
    float dst = hit ? tNear > 0 ? tNear : 0 : INFINITY;
    return dst;
}

HitInfo getHitInfo(Ray ray) {
    HitInfo info;
    info.dist = INFINITY;
    info.triangleIndex = -1;
/* Old code, loop over all triangles
    for (int i = 0; i < triangles.length(); i++) {
        float triangleDist = getRayTriangleDistance(ray, i);
        if (triangleDist < info.dist) {
            info.dist = triangleDist;
            info.triangleIndex = i;
        }
    }
    return info;
*/
    int stack[MAX_BVH_TRAVERSAL_STACK_SIZE];
    int i = 0;
    stack[0] = 0;
    while (i > -1) {
        int bvhIndex = stack[i--];
        if (rayBoundingBoxDist(ray, bvh[bvhIndex][0], bvh[bvhIndex][1]) >= info.dist) {
            continue;
        }
        bool isLeaf = abs(bvh[bvhIndex][2].x - 1.0f) <= EPS;
        if (isLeaf) {
            int triangleStart = int(bvh[bvhIndex][2].y);
            int triangleEnd = int(bvh[bvhIndex][2].z);
            for (int i = triangleStart; i <= triangleEnd; i++) {
                float triangleDist = getRayTriangleDistance(ray, i);
                if (triangleDist < info.dist) {
                    info.dist = triangleDist;
                    info.triangleIndex = i;
                }
            }
        } else {
            int child1 = int(bvh[bvhIndex][2].y);
            int child2 = int(bvh[bvhIndex][2].z);
            float d1 = rayBoundingBoxDist(ray, bvh[child1][0], bvh[child1][1]);
            float d2 = rayBoundingBoxDist(ray, bvh[child2][0], bvh[child2][1]);
            if (d1 < d2) {
                if (d1 < info.dist)
                    stack[++i] = child1;
                if (d2 < info.dist)
                    stack[++i] = child2;
            } else {
                if (d2 < info.dist)
                    stack[++i] = child2;
                if (d1 < info.dist)
                    stack[++i] = child1;
            }
        }
    }
    return info;
}

vec4 getColour(Ray ray, uint bouncesLeft) {
    HitInfo info = getHitInfo(ray);
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
    Ray ray;
    ray.dir = cameraRotation * dir;
    ray.origin = cameraPos + ray.dir;
    ray.invDir = vec3(1.0 / ray.dir.x, 1.0 / ray.dir.y, 1.0 / ray.dir.z);
    vec4 colour = BLACK;
    for (uint i = uint(0); i < u_RaysPerPixel; i++) {
        colour += getColour(ray, u_RayBounces);
    }
    colour /= u_RaysPerPixel;
    imageStore(pixelOutput, ivec2(gl_GlobalInvocationID.xy), colour);
}