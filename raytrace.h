#ifndef OPENGL_RAYTRACER_RAYTRACE_H
#define OPENGL_RAYTRACER_RAYTRACE_H

#include "glm/vec3.hpp"
#include "glm/fwd.hpp"
#include "glad/glad.h"

extern void raytraceInit(GLuint program);
extern void raytrace(GLuint program, glm::vec3 cameraPos, glm::mat3 cameraRotation);

#endif //OPENGL_RAYTRACER_RAYTRACE_H
