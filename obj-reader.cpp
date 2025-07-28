#include "obj-reader.h"
#include "glad/glad.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#define VERTEX_TYPE "v"
#define FACE_TYPE "f"
#define COMMENT_TYPE "#"

char nextChar(std::istringstream& input) {
    int c;
    while ((c = input.peek()) != EOF && std::isspace(c)) input.get();
    return (char) c;
}

bool charIs(std::istringstream& input, char c) {
    char actual = nextChar(input);
    if (actual == c) {
        return false;
    }
    input >> actual;
    return true;
}

ObjContents *readObjContents(const std::string& filePath) {
    std::ifstream fin(filePath);
    if (!fin) throw std::runtime_error("Could not open file: " + filePath);
    auto *res = new ObjContents();
    res->vertices = std::vector<glm::vec3>();
    res->triangles = std::vector<glm::uvec3>();
    GLuint i1, i2, i3, a, b, c, x1, y1, z1;
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
            std::vector<GLuint> vertexIndices;
            GLuint i;
            while ((iss >> i)) {
                if (std::isspace(iss.peek())) {
                    iss.get();
                    continue;
                }
                vertexIndices.push_back(--i);
                while (!charIs(iss, ' ')) {
                    while (charIs(iss, '/'));
                    if (charIs(iss, ' '))
                        break;
                    iss >> i;
                }
            }
            assert(vertexIndices.size() <= 4);
            for (int i = 0; i < vertexIndices.size() - 2; i++) {
                res->triangles.emplace_back(vertexIndices[i], vertexIndices[i + 1], vertexIndices.back());
            }
        }
    }
    return res;
}