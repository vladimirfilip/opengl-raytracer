#version 430

layout(local_size_x = 32, local_size_y = 32) in;

uniform float u_ScreenWidth;
uniform float u_ScreenHeight;
uniform float u_FOV;
uniform float u_ViewportDist;
uniform uint u_RayBounces;
uniform uint u_RaysPerPixel;
uniform vec3 cameraPos;
uniform vec2 cameraRotation;

layout(std430, binding = 0) buffer VertexBuffer {
    vec4 vertices[];
};

layout(std430, binding = 1) buffer TriangleBuffer {
    uvec4 triangles[];
};

layout(std430, binding = 2) buffer NormalsBuffer {
    vec4 triangleNormals[];
};

layout(binding = 3, rgba32f) uniform image2D pixelOutput;


const float EPS = 1e-3;

#define WHITE vec4(1.0f, 1.0f, 1.0f, 1.0f)
#define BLACK vec4(0.0f, 0.0f, 0.0f, 1.0f)

bool intersectsTriangle(vec3 origin, vec3 dir, int triangleIndex) {
    vec3 a = vertices[triangles[triangleIndex].x].xyz;
    vec3 b = vertices[triangles[triangleIndex].y].xyz;
    vec3 c = vertices[triangles[triangleIndex].z].xyz;
    vec3 n = triangleNormals[triangleIndex].xyz;
    float denom = dot(dir, n);
    if (abs(denom) < EPS)
        return false;
    float alpha = dot(a - origin, n) / denom;
    if (alpha <= EPS)
        return false;
    // x = point on the ray that intersects the plane of the triangle
    vec3 x = origin + alpha * dir;
    // x is inside the triangle if sum of areas of triangles formed by x and each pair of triangle vertices equals
    // the area of the triangle
    float area = length(cross(x - a, x - b)) + length(cross(x - b, x - c)) + length(cross(x - c, x - a));
    return abs(area - length(n)) < EPS;
}

vec4 castRay(vec3 origin, vec3 dir, uint bouncesLeft) {
    for (int i = 0; i < triangles.length(); i++) {
        if (intersectsTriangle(origin, dir, i)) {
            return WHITE;
        }
    }
    return BLACK;
}

void main() {
    if (gl_GlobalInvocationID.x >= u_ScreenWidth || gl_GlobalInvocationID.y >= u_ScreenHeight) {
        return;
    }
    // tan (FOV / 2) = (viewportWidth / 2) / viewportDist
    float pixelWidth = tan(u_FOV / 2.0f) * u_ViewportDist * 2.0f / u_ScreenWidth;
    vec3 dir = vec3(
        (float(gl_GlobalInvocationID.x) - u_ScreenWidth / 2.0f) * pixelWidth,
        (float(gl_GlobalInvocationID.y) - u_ScreenHeight / 2.0f) * pixelWidth,
        u_ViewportDist
    );
    mat3 xRotation = mat3(
        1.0f, 0.0f, 0.0f,
        0.0f, cos(cameraRotation.x), -sin(cameraRotation.x),
        0.0f, sin(cameraRotation.x), cos(cameraRotation.x)
    );
    mat3 yRotation = mat3(
        cos(cameraRotation.y), 0.0f, sin(cameraRotation.y),
        0.0f, 1.0f, 0.0f,
        -sin(cameraRotation.y), 0.0f, cos(cameraRotation.y)
    );
    vec3 rotatedDir = yRotation * xRotation * dir;
    vec3 rayOrigin = cameraPos + rotatedDir;
    vec4 colour = BLACK;
    for (uint i = uint(0); i < u_RaysPerPixel; i++) {
        colour += castRay(rayOrigin, rotatedDir, u_RayBounces);
    }
    colour /= u_RaysPerPixel;
    imageStore(pixelOutput, ivec2(gl_GlobalInvocationID.xy), colour);
}