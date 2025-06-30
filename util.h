#ifndef OPENGL_RAYTRACER_UTIL_H
#define OPENGL_RAYTRACER_UTIL_H
#pragma once
#include "glad/glad.h"

extern void checkGLError(const std::string& label);

extern std::string loadShaderCodeFromFile(const std::string& filePath);

extern GLuint importAndCompileShader(const std::string& path, GLenum shaderType);

template<typename... Args>
GLuint generateProgram(Args... shaders) {
    GLuint program = glCreateProgram();
    (glAttachShader(program, shaders), ...);
    glLinkProgram(program);
    int success;
    char infoLog[512];
    // check for linking errors
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    (glDeleteShader(shaders), ...);
    return program;
}

#endif // OPENGL_RAYTRACER_UTIL_H
