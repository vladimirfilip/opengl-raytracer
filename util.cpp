#include <iostream>
#include <fstream>
#include <sstream>

#include "util.h"
#include "glad/glad.h"

// Utility function for error checking
void checkGLError(const std::string& label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[OpenGL Error] (" << err << ") at " << label << std::endl;
    }
}

std::string loadShaderCodeFromFile(const std::string& filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filePath);
    }

    std::ostringstream shaderStream;
    shaderStream << shaderFile.rdbuf();

    return shaderStream.str();
}

GLuint importAndCompileShader(const std::string& path, GLenum shaderType) {
    std::string shaderCode = loadShaderCodeFromFile(path);
    const char *shaderSource = shaderCode.c_str();
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &shaderSource, nullptr);
    glCompileShader(shader);
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cout << "ERROR::" + std::to_string(shaderType) + "::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    return shader;
}
