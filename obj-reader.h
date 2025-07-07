#ifndef OPENGL_RAYTRACER_OBJ_READER_H
#define OPENGL_RAYTRACER_OBJ_READER_H

#include <vector>
#include <string>
#include "glm/glm.hpp"

struct ObjContents {
    std::vector<glm::vec3> vertices;
    std::vector<glm::uvec3> triangles;
};

extern ObjContents* readObjContents(const std::string& filePath);

#endif //OPENGL_RAYTRACER_OBJ_READER_H
