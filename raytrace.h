#ifndef OPENGL_RAYTRACER_RAYTRACE_H
#define OPENGL_RAYTRACER_RAYTRACE_H

#include "glm/vec3.hpp"

extern void raytraceInit();
extern void raytrace(glm::vec3 cameraPos, double mouseX, double mouseY);

#endif //OPENGL_RAYTRACER_RAYTRACE_H
