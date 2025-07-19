#ifndef OPENGL_RAYTRACER_RAYTRACE_H
#define OPENGL_RAYTRACER_RAYTRACE_H

#include "glm/vec3.hpp"
#include "glm/fwd.hpp"
#include "glad/glad.h"

extern GLuint raytraceInit();
extern void raytrace(glm::vec3 cameraPos, glm::mat3 cameraRotation, int renderMode, int num_groups_x, int num_groups_y);

#endif //OPENGL_RAYTRACER_RAYTRACE_H
