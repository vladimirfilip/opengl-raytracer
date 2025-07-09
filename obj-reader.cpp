#include "obj-reader.h"
#include "glad/glad.h"

#include <iostream>
#include <fstream>
#include <sstream>

ObjContents *readObjContents(const std::string& filePath) {
    std::ifstream fin(filePath);
    if (!fin) throw std::runtime_error("Could not open file: " + filePath);
    auto *res = new ObjContents();
    res->vertices = std::vector<glm::vec3>();
    res->triangles = std::vector<glm::uvec3>();

    std::string line;
    while (std::getline(fin, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        // Skip comments
        if (line[0] == '#') continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "v") {
            float x, y, z;
            if (!(iss >> x >> y >> z)) continue; // skip malformed line
            res->vertices.emplace_back(x, y, z);
        } else if (type == "f") {
            unsigned int i1, i2, i3;
            if (!(iss >> i1 >> i2 >> i3)) continue; // skip malformed line
            // OBJ is 1-based, C++ is 0-based
            res->triangles.emplace_back(i1 - 1, i2 - 1, i3 - 1);
        } // else: ignore anything else (vn, vt, etc)
    }
    return res;
}