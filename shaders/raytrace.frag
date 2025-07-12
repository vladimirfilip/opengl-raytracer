#version 430

#define WHITE vec4(1.0f, 1.0f, 1.0f, 1.0f)
#define BLACK vec4(0.0f, 0.0f, 0.0f, 1.0f)
#define INFINITY 1.0 / 0.0
#define MAX_BVH_TRAVERSAL_STACK_SIZE 128

#define RENDER_MODE 1
#define TRIANGLE_TEST_MODE 2
#define BOX_TEST_MODE 3

uniform float u_ScreenWidth;
uniform float u_ScreenHeight;
uniform float u_FOV;
uniform float u_ViewportDist;
uniform uint u_RayBounces;
uniform uint u_RaysPerPixel;
uniform vec3 cameraPos;
uniform mat3 cameraRotation;

uniform uint renderMode;

layout(std430, binding = 1) buffer TriangleBuffer {
    mat3 triangles[];
};

layout(std430, binding = 2) buffer NormalsBuffer {
    vec3 triangleNormals[];
};

layout(std430, binding = 3) buffer BVHBuffer {
    mat3 bvh[];
};

layout(std430, binding = 4) buffer TriangleColourBuffer {
    vec4 triangleColours[];
};


struct HitInfo {
    float dist;
    int triangleIndex;
};

struct Ray {
    vec3 origin, dir, invDir;
};

int numBoxTests = 0;
int boxTestsMax = 1000;
vec4 boxTestsColour = vec4(0.0f, 1.0f, 1.0f, 1.0f);

int numTriangleTests = 0;
int triangleTestsMax = 500;
vec4 triangleTestsColour = vec4(1.0f, 1.0f, 0.0f, 0.0f);

const float EPS = 1e-6;

/*
 * Gets distance the ray has to travel before hitting the triangle
 * distance = INFINITY if ray does not intersect
 * Uses well-known Möller-Trumbore algorithm
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
 */
float getRayTriangleDistance(Ray ray, int triangleIndex) {
    vec3 a = triangles[triangleIndex][0];
    vec3 b = triangles[triangleIndex][1];
    vec3 c = triangles[triangleIndex][2];

    vec3 edge1 = b - a;
    vec3 edge2 = c - a;
    vec3 ray_cross_e2 = cross(ray.dir, edge2);
    float det = dot(edge1, ray_cross_e2);

    if (abs(det) < EPS)
        return INFINITY;    // This ray is parallel to this triangle.

    float inv_det = 1.0 / det;
    vec3 s = ray.origin - a;
    float u = inv_det * dot(s, ray_cross_e2);

    if ((u < 0 && abs(u) > EPS) || (u > 1 && abs(u-1) > EPS))
        return INFINITY;

    vec3 s_cross_e1 = cross(s, edge1);
    float v = inv_det * dot(ray.dir, s_cross_e1);

    if ((v < 0 && abs(v) > EPS) || (u + v > 1 && abs(u + v - 1) > EPS))
        return INFINITY;

    // At this stage we can compute t to find out where the intersection point is on the line.
    float t = inv_det * dot(edge2, s_cross_e1);

    return t > EPS ? t : INFINITY;
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
    numBoxTests++;
    if (rayBoundingBoxDist(ray, bvh[0][0], bvh[0][1]) == INFINITY) {
        return info;
    }
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
    float dist[MAX_BVH_TRAVERSAL_STACK_SIZE];
    int i = 0;
    stack[0] = 0;
    dist[0] = rayBoundingBoxDist(ray, bvh[0][0], bvh[0][1]);
    while (i > -1) {
        int bvhIndex = stack[i];
        if (dist[i--] >= info.dist) {
            continue;
        }
        bool isLeaf = abs(bvh[bvhIndex][2].x - 1.0f) <= EPS;
        if (isLeaf) {
            int triangleStart = int(bvh[bvhIndex][2].y);
            int triangleEnd = int(bvh[bvhIndex][2].z);
            for (int i = triangleStart; i <= triangleEnd; i++) {
                numTriangleTests++;
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
            numBoxTests += 2;
            if (d1 < d2) {
                if (d2 < info.dist) {
                    stack[++i] = child2;
                    dist[i] = d2;
                }
                if (d1 < info.dist) {
                    stack[++i] = child1;
                    dist[i] = d1;
                }
            } else {
                if (d1 < info.dist) {
                    stack[++i] = child1;
                    dist[i] = d1;
                }
                if (d2 < info.dist) {
                    stack[++i] = child2;
                    dist[i] = d2;
                }
            }
        }
    }
    return info;
}

vec4 getColour(Ray ray, uint bouncesLeft) {
    HitInfo info = getHitInfo(ray);
    return info.dist < INFINITY ? WHITE * abs(dot(ray.dir, triangleNormals[info.triangleIndex])) : BLACK;
}

out vec4 FragColor;

void main() {
    // tan (FOV / 2) = (viewportWidth / 2) / viewportDist
    float pixelWidth = tan(u_FOV / 2.0f) * u_ViewportDist * 2.0f / u_ScreenWidth;
    vec3 dir = cameraRotation * vec3(
        (gl_FragCoord.x - u_ScreenWidth / 2.0f) * pixelWidth,
        (gl_FragCoord.y - u_ScreenHeight / 2.0f) * pixelWidth,
        u_ViewportDist
    );
    Ray ray;
    ray.dir = normalize(dir);
    ray.origin = cameraPos + dir;
    ray.invDir = 1.0f / ray.dir;
    vec4 colour = BLACK;
    for (uint i = uint(0); i < u_RaysPerPixel; i++) {
        colour += getColour(ray, u_RayBounces);
    }
    colour /= u_RaysPerPixel;
    if (renderMode == TRIANGLE_TEST_MODE) {
        colour = float(min(triangleTestsMax, numTriangleTests)) / float(triangleTestsMax) * triangleTestsColour;
    } else if (renderMode == BOX_TEST_MODE) {
        colour = float(min(boxTestsMax, numBoxTests)) / float(boxTestsMax) * boxTestsColour;
    }
    FragColor = colour;
}