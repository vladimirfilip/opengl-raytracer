#version 430

layout(local_size_x = 1) in;

layout(std430, binding = 0) buffer VertexBuffer {
    vec4 vertices[];
};

layout(std430, binding = 1) buffer TriangleBuffer {
    uvec4 triangles[];
};

layout(std430, binding = 2) buffer NormalBuffer {
    vec4 normals[];
};

void main() {
    uint i = gl_GlobalInvocationID.x;
    vec3 a = vertices[triangles[i].x].xyz;
    vec3 b = vertices[triangles[i].y].xyz;
    vec3 c = vertices[triangles[i].z].xyz;
    normals[i] = vec4(cross(a - b, a - c), 0.0f);
}