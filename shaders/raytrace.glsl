#version 430

layout(local_size_x = 16, local_size_y = 16) in;

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
uniform uint frameCount;

uint randSeed;

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

layout(binding = 5, rgba32f) uniform image2D pixelOutput;


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

#define PI 3.1415926

const float EPS = 1e-6;

/*
 * Gets distance the ray has to travel before hitting the triangle
 * distance = INFINITY if ray does not intersect
 * Uses well-known Möller-Trumbore algorithm
 * https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
 */
float getRayTriangleDistance(Ray ray, int triangleIndex) {
    numTriangleTests++;
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
    numBoxTests++;
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

/*
 * Thanks to Jacob Gordiak for skybox + random direction code and explanation of colour reflection code
 * https://github.com/jakubg05/Ray-Tracing/blob/main/core/resources/shaders/ComputeRayTracing.comp
 */

vec3 sampleSkybox(vec3 dir) {
    vec3 skyGroundColour  = vec3(0.6392156862f, 0.5803921f, 0.6392156862f);
    vec3 skyColourHorizon = vec3(1.0f, 1.0f, 1.0f);
    vec3 skyColourZenith  = vec3(0.486274509f, 0.71372549f, 234.0f / 255.0f);
    float skyGradientT = pow(smoothstep(0.0, 0.4, dir.y), 0.35);
    float groundToSkyT = smoothstep(-0.01, 0.0, dir.y);
    vec3 skyGradient = mix(skyColourHorizon.rgb, skyColourZenith.rgb, skyGradientT);
    skyGradient = mix(skyGroundColour.rgb, skyGradient, groundToSkyT);
    return skyGradient;
}

float rand()
{
    randSeed = randSeed * 747796405u + 2891336453u;
    uint result = ((randSeed >> ((randSeed >> 28u) + 4u)) ^ randSeed) * 277803737u;
    result = (result >> 22u) ^ result;
    return float(result) / 4294967295.0;
}

float randNormalDistribution() {
    float theta = 2 * PI * rand();
    float rho = sqrt(-2 * log(rand()));
    return rho * cos(theta);
}

vec3 randDirection() {
    float x = randNormalDistribution();
    float y = randNormalDistribution();
    float z = randNormalDistribution();
    return normalize(vec3(x, y, z));
}

vec3 randDirectionInHemisphere(vec3 normal) {
    vec3 randomDirection = randDirection();
    if (dot(normal, randomDirection) < 0.0)
        randomDirection = -randomDirection;
    return randomDirection;
}

vec4 getColour(Ray ray, uint bouncesLeft) {
    vec3 rayColour = vec3(1.0);
    vec3 result = vec3(0.0);
    for (uint i = 0; i < bouncesLeft; i++) {
        HitInfo info = getHitInfo(ray);
        if (info.dist < INFINITY) {
            ray.origin += ray.dir * info.dist;
            vec3 normal = triangleNormals[info.triangleIndex];
            ray.dir = randDirectionInHemisphere(normal);
        } else {
            result += sampleSkybox(ray.dir) * rayColour;
            break;
        }
    }
    return vec4(result, 1.0f);
}

void main() {
    if (gl_GlobalInvocationID.x >= u_ScreenWidth || gl_GlobalInvocationID.y >= u_ScreenHeight) {
        return;
    }
    uint pixelIndex = uint(gl_GlobalInvocationID.y * u_ScreenWidth + gl_GlobalInvocationID.x);
    randSeed = pixelIndex + frameCount * 745621;
    // tan (FOV / 2) = (viewportWidth / 2) / viewportDist
    float pixelWidth = tan(u_FOV / 2.0f) * u_ViewportDist * 2.0f / u_ScreenWidth;
    vec3 dir = cameraRotation * vec3(
        (gl_GlobalInvocationID.x - u_ScreenWidth / 2.0f) * pixelWidth,
        (gl_GlobalInvocationID.y - u_ScreenHeight / 2.0f) * pixelWidth,
        u_ViewportDist
    );
    Ray ray;
    ray.dir = normalize(dir);
    ray.origin = cameraPos;
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
    imageStore(pixelOutput, ivec2(gl_GlobalInvocationID.xy), colour);
}