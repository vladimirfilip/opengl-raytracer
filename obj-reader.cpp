#include "obj-reader.h"
#include "glad/glad.h"

#include <iostream>
#include <fstream>

#define VERTEX_TYPE 'v'
#define FACE_TYPE 'f'

ObjContents *readObjContents(const std::string& filePath) {
    std::ifstream fin(filePath);
    auto *res = new ObjContents();
    res->vertices = std::vector<glm::vec4>();
    res->triangles = std::vector<glm::uvec4>();
    char type;
    GLuint i1, i2, i3;
    GLfloat x, y, z;
    while (!fin.eof()) {
        fin >> type;
        switch (type) {
            case VERTEX_TYPE:
                fin >> x >> y >> z;
                res->vertices.push_back(glm::vec4(x, y, z, 1.0f));
                break;
            case FACE_TYPE:
                fin >> i1 >> i2 >> i3;
                res->triangles.push_back(glm::uvec4(i1, i2, i3, 0));
                break;
            default:
                throw std::runtime_error("Unknown type: " + std::string(1, type));
                break;
        }
    }
    return res;
}
