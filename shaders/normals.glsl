#version 430

layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer VertexBuffer {
    vec3 vertices[];
};

layout(std430, binding = 1) buffer TriangleBuffer {
    uvec3 triangles[];
};

layout(std430, binding = 2) buffer NormalBuffer {
    vec3 normals[];
};

void main() {
    uint i = gl_GlobalInvocationID.x;
    vec3 a = vertices[triangles[i].x];
    vec3 b = vertices[triangles[i].y];
    vec3 c = vertices[triangles[i].z];
    normals[i] = cross(a - b, a - c);
}