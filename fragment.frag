#version 430 core

uniform float u_ScreenWidth;
uniform float u_ScreenHeight;
uniform float u_FOV;
uniform float u_ViewportDist;
uniform uint u_RayBounces;
uniform uint u_RaysPerPixel;

layout(std430, binding = 0) buffer VertexBuffer {
    vec4 vertices[];
};

layout(std430, binding = 1) buffer TriangleBuffer {
    uvec4 triangles[];
};

const float EPS = 1e-3;

#define WHITE vec4(1.0f, 1.0f, 1.0f, 1.0f)
#define BLACK vec4(0.0f, 0.0f, 0.0f, 1.0f)

bool intersectsTriangle(vec3 origin, vec3 dir, int triangleIndex) {
    vec3 a = vertices[triangles[triangleIndex].x].xyz;
    vec3 b = vertices[triangles[triangleIndex].y].xyz;
    vec3 c = vertices[triangles[triangleIndex].z].xyz;
    vec3 n = cross(a - b, a - c);
    // ((origin + alpha * dir) - a) dot n = 0
    // (origin + alpha * dir) dot n = a dot n
    // alpha * dir dot n  = (a - origin) dot n
    // alpha = ((a - origin) dot n) / (dir dot n)
    float denom = dot(dir, n);
    if (abs(denom) < EPS)
        return false;
    float alpha = dot(a - origin, n) / denom;
    if (alpha <= EPS)
        return false;
    vec3 x = origin + alpha * dir;
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

out vec4 FragColor;
void main() {
    // tan (FOV / 2) = (viewportWidth / 2) / viewportDist
    float pixelWidth = tan(u_FOV / 2.0f) * u_ViewportDist * 2.0f / u_ScreenWidth;
    vec3 dir = {
        (gl_FragCoord.x - float(u_ScreenWidth) / 2.0f) * pixelWidth,
        (gl_FragCoord.y - float(u_ScreenHeight) / 2.0f) * pixelWidth,
        u_ViewportDist
    };
    vec4 colour = {0.0f, 0.0f, 0.0f, 0.0f};
    for (uint i = uint(0); i < u_RaysPerPixel; i++) {
        colour += castRay(dir, dir, u_RayBounces);
    }
    colour /= u_RaysPerPixel;
    FragColor = colour;
}