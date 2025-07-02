#version 430 core
layout (location = 0) in vec3 aPos;
out vec2 texCoords;
void main()
{
   texCoords = (aPos.xy + 1.0f) / 2.0f;
   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}