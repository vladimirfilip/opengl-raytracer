#version 430

layout(local_size_x = 1) in;

layout(std430, binding = 1) buffer TriangleBuffer {
    mat3 triangles[];
};

layout(std430, binding = 2) buffer NormalBuffer {
    vec3 normals[];
};

void main() {
    uint i = gl_GlobalInvocationID.x;
    vec3 a = triangles[i][0];
    vec3 b = triangles[i][1];
    vec3 c = triangles[i][2];
    normals[i] = normalize(cross(a - b, a - c));
}