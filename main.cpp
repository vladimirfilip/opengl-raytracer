#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <numbers>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#define VERTEX_SSBO_BINDING 0
#define TRIANGLE_SSBO_BINDING 1
#define TRIANGLE_NORMAL_SSBO_BINDING 2

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

// Utility function for error checking
void checkGLError(const std::string& label) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "[OpenGL Error] (" << err << ") at " << label << std::endl;
    }
}

template <typename T>
void initSSBO(std::vector<T>& data, unsigned int binding, unsigned int* ssbo) {
    glGenBuffers(1, ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glGenBuffers");
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, *ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBindBuffer");
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 data.size() * sizeof(T),
                 data.data(),
                 GL_STATIC_DRAW);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBufferData");
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, *ssbo);
    checkGLError("(initSSBO, binding " + std::to_string(binding) + ") glBindBufferBase");
}

void runComputeShader(GLuint shader, int num_groups_x, int num_groups_y, int num_groups_z) {
    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glUseProgram(program);
    glDispatchCompute(num_groups_x, num_groups_y, num_groups_z);
    glDeleteProgram(program);
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

    GLuint vertexShader = importAndCompileShader("../vertex.vert", GL_VERTEX_SHADER);
    GLuint fragmentShader = importAndCompileShader("../fragment.frag", GL_FRAGMENT_SHADER);

    // set up screen-size quad vertex data (and buffer(s)) and configure vertex attributes
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
            {-1.0f, 1.0f, 10.0f, 1.0f},
            {1.0f, 1.0f, 10.0f, 1.0f},
            {0.0f, -1.0f, 10.0f, 1.0f},
            {3.0f, 1.0f, 10.0f, 1.0f},
            {5.0f, 1.0f, 7.0f, 1.0f},
            {4.0f, -1.0f, 8.0f, 1.0f},
            {-3.0f, 1.0f, 10.0f, 1.0f},
            {-5.0f, 1.0f, 7.0f, 1.0f},
            {-4.0f, -1.0f, 8.0f, 1.0f},
    };
    unsigned int vertexSSBO;
    initSSBO(triangleVertices, VERTEX_SSBO_BINDING, &vertexSSBO);
    std::vector<glm::uvec4> triangles = {
            {0, 1, 2, 0},
            {3, 4, 5, 0},
            {6, 7, 8, 0},
    };
    initSSBO(triangles, TRIANGLE_SSBO_BINDING, &vertexSSBO);
    unsigned int triangleNormalsSSBO;
    std::vector<glm::vec4> triangleNormals(triangles.size());
    initSSBO(triangleNormals, TRIANGLE_NORMAL_SSBO_BINDING, &triangleNormalsSSBO);
    GLuint normalShader = importAndCompileShader("../normals.glsl", GL_COMPUTE_SHADER);
    runComputeShader(normalShader, triangles.size(), 1, 1);
    // Static constants cast to float
    auto u_screenWidth = static_cast<GLfloat>(mode->width);
    auto u_screenHeight = static_cast<GLfloat>(mode->height);
    auto u_FOV = static_cast<GLfloat>(FOV * std::numbers::pi / 180.f);

    // link shaders
    GLuint drawCallProgram = glCreateProgram();
    glAttachShader(drawCallProgram, vertexShader);
    glAttachShader(drawCallProgram, fragmentShader);
    glLinkProgram(drawCallProgram);
    int success;
    char infoLog[512];
    // check for linking errors
    glGetProgramiv(drawCallProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(drawCallProgram, 512, nullptr, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(drawCallProgram);
    glUniform1f(
            glGetUniformLocation(drawCallProgram, "u_ScreenWidth"),
            u_screenWidth);
    glUniform1f(
            glGetUniformLocation(drawCallProgram, "u_ScreenHeight"),
            u_screenHeight);
    glUniform1f(glGetUniformLocation(drawCallProgram, "u_FOV"),
                u_FOV);
    glUniform1f(glGetUniformLocation(drawCallProgram, "u_ViewportDist"),
                VIEWPORT_DIST);
    glUniform1ui(glGetUniformLocation(drawCallProgram, "u_RaysPerPixel"), RAYS_PER_PIXEL);
    glUniform1ui(glGetUniformLocation(drawCallProgram, "u_RayBounces"), RAY_BOUNCES);

    // render loop
    // -----------
    int frameCount = 0;
    double startTime = glfwGetTime();
    checkGLError("Pre-render-loop");
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        checkGLError("After process input");
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(window);
        glfwPollEvents();
        frameCount++;
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(drawCallProgram);
    std::cout << "Average FPS: " << frameCount / (glfwGetTime() - startTime) << "\n";
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
