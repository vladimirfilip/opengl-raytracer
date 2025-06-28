#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <GL/gl.h>

#include <numbers>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <vector>

/*
 * Disclaimer: boilerplate to render two triangles on the screen
 * is imported from https://learnopengl.com/Getting-started/Hello-Triangle
 */

void processInput(GLFWwindow *window);

// settings
const float FOV = 90.0f;
const float VIEWPORT_DIST = 1.0f;
const unsigned int RAY_BOUNCES = 5;
const unsigned int RAYS_PER_PIXEL = 5;

const char *vertexShaderSource = "#version 430 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "}\0";

std::string loadShaderFromFile(const std::string& filePath) {
    std::ifstream shaderFile(filePath);
    if (!shaderFile.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filePath);
    }

    std::ostringstream shaderStream;
    shaderStream << shaderFile.rdbuf();

    return shaderStream.str();
}

// Utility function for error checking
void checkGLError(const char* label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[OpenGL Error] (" << err << ") at " << label << std::endl;
        // Optionally, decode err to a string
    }
}


int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfw window creation
    // --------------------
    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    GLFWwindow *window = glfwCreateWindow(mode->width, mode->height, "opengl raytracer", monitor, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    std::string fragmentShaderContents = loadShaderFromFile("../fragment.frag");
    const char *fragmentShaderSource = fragmentShaderContents.c_str();
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
            1.0f, 1.0f, 0.0f,  // top right
            1.0f, -1.0f, 0.0f,  // bottom right
            -1.0f, -1.0f, 0.0f,  // bottom left
            -1.0f, 1.0f, 0.0f   // top left
    };
    unsigned int indices[] = {  // note that we start from 0!
            0, 1, 3,  // first Triangle
            1, 2, 3   // second Triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *) nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Building internal triangle and vertex buffers
    std::vector<glm::vec4> triangleVertices = {
            {-1.0f, 1.0f, 8.0f, 1.0f},
            {1.0f, 1.0f, 10.0f, 1.0f},
            {0.0f, -1.0f, 10.0f, 1.0f},
    };
    unsigned int vertexSSBO;
    glGenBuffers(1, &vertexSSBO);
    checkGLError("(183) glGenBuffers");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSBO);
    checkGLError("glBindBuffer");
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 triangleVertices.size() * sizeof(glm::vec4),
                 triangleVertices.data(),
                 GL_STATIC_DRAW);
    checkGLError("glBufferData");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexSSBO); // Bind to binding = 0
    checkGLError("glBindBufferBase");
    std::vector<glm::uvec4> triangles = {
            {0, 1, 2, 0},
    };
    unsigned int triangleSSBO;
    glGenBuffers(1, &triangleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 triangles.size() * sizeof(glm::uvec4),
                 triangles.data(),
                 GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangleSSBO); // Bind to binding = 1
    checkGLError("glBindBufferBase");
    // Static constants cast to float
    auto u_screenWidth = static_cast<GLfloat>(mode->width);
    auto u_screenHeight = static_cast<GLfloat>(mode->height);
    auto u_FOV = static_cast<GLfloat>(FOV * std::numbers::pi / 180.f);

    // render loop
    // -----------
    int frameCount = 0;
    double startTime = glfwGetTime();
    checkGLError("Pre-render-loop");
    while (!glfwWindowShouldClose(window)) {
        // input
        // -----
        processInput(window);
        checkGLError("After process input");
        // render
        // ------
        glUseProgram(shaderProgram);
        glUniform1f(
                glGetUniformLocation(shaderProgram, "u_ScreenWidth"),
                u_screenWidth);
        glUniform1f(
                glGetUniformLocation(shaderProgram, "u_ScreenHeight"),
                u_screenHeight);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_FOV"),
                    u_FOV);
        glUniform1f(glGetUniformLocation(shaderProgram, "u_ViewportDist"),
                    VIEWPORT_DIST);
        glUniform1ui(glGetUniformLocation(shaderProgram, "u_RaysPerPixel"), RAYS_PER_PIXEL);
        glUniform1ui(glGetUniformLocation(shaderProgram, "u_RayBounces"), RAY_BOUNCES);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        frameCount++;
        if (glfwGetTime() - startTime >= 1.0) {
            std::cout << "FPS: " << frameCount << "\n";
            frameCount = 0;
            startTime += 1.0;
        }
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
