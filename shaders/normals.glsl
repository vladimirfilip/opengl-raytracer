#version 430

layout(local_size_x = 1) in;

layout(std430, binding = 7) buffer TriV0Buffer { vec4 triv0[]; };
layout(std430, binding = 8) buffer TriV1Buffer { vec4 triv1[]; };
layout(std430, binding = 9) buffer TriV2Buffer { vec4 triv2[]; };

layout(std430, binding = 2) buffer NormalBuffer {
    vec3 normals[];
};

void main() {
    uint i = gl_GlobalInvocationID.x;
    vec3 a = triv0[i].xyz;
    vec3 b = triv1[i].xyz;
    vec3 c = triv2[i].xyz;
    normals[i] = normalize(cross(a - b, a - c));
}