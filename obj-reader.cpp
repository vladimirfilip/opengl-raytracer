#include "obj-reader.h"
#include "glad/glad.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#define VERTEX_TYPE "v"
#define FACE_TYPE "f"
#define COMMENT_TYPE "#"

ObjContents *readObjContents(const std::string &filePath) {
    std::ifstream fin(filePath);
    if (!fin) throw std::runtime_error("Could not open file: " + filePath);
    auto *res = new ObjContents();
    res->vertices = std::vector<glm::vec3>();
    res->triangles = std::vector<glm::uvec3>();
    GLuint i1, i2, i3, a, b, c;
    GLfloat x, y, z;
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == COMMENT_TYPE)
            continue;
        else if (type == VERTEX_TYPE) {
            if (!(iss >> x >> y >> z)) continue; // skip malformed line
            res->vertices.emplace_back(x, y, z);
        } else if (type == FACE_TYPE) {
            if (sscanf(line.c_str(), "f %d//%d %d//%d %d//%d", &i1, &a, &i2, &b, &i3, &c) == 6 ||
                sscanf(line.c_str(), "f %d %d %d", &i1, &i2, &i3) == 3) {
                res->triangles.emplace_back(--i1, --i2, --i3);
            }
        }
    }
    return res;
}