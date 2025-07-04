#ifndef OPENGL_RAYTRACER_RAYTRACE_H
#define OPENGL_RAYTRACER_RAYTRACE_H

#include "glm/vec3.hpp"
#include "glm/fwd.hpp"

extern void raytraceInit();
extern void raytrace(glm::vec3 cameraPos, glm::mat3 cameraRotation);

#endif //OPENGL_RAYTRACER_RAYTRACE_H
