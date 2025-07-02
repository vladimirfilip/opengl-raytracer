#ifndef OPENGL_RAYTRACER_OBJ_READER_H
#define OPENGL_RAYTRACER_OBJ_READER_H

#include <vector>
#include <string>
#include "glm/vec4.hpp"

struct ObjContents {
    std::vector<glm::vec4> vertices;
    std::vector<glm::uvec4> triangles;
};

ObjContents* readObjContents(const std::string& filePath);

#endif //OPENGL_RAYTRACER_OBJ_READER_H
