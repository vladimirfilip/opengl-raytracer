#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>

#include "util.h"
#include "raytrace.h"
#include "constants.h"
#include "obj-reader.h"
#include "bvh.h"

/*
 * Disclaimer: boilerplate to render two triangles on the screen
 * is imported from https://learnopengl.com/Getting-started/Hello-Triangle
 */

static glm::vec3 cameraPos(0.0f, 0.0f, 0.0f);
static glm::mat3 cameraRotation;
static double cameraPitch = 0.0f, cameraYaw = 0.0f;
static double prevMouseX, prevMouseY;
static int screenWidth, screenHeight;
static GLFWwindow* window;

void processInput(double &prevTime);

static void updateCameraRotation() {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    double dx = mouseX - prevMouseX;
    double dy = mouseY - prevMouseY;
    cameraPitch -= dy * std::numbers::pi / screenHeight;
    cameraYaw -= dx * std::numbers::pi / screenWidth;
    cameraPitch = std::max(-std::numbers::pi / 2.0f, std::min(std::numbers::pi / 2.0f, cameraPitch));
    prevMouseX = mouseX;
    prevMouseY = mouseY;
    glm::mat3 xRotation = glm::mat3(
            1.0f, 0.0f, 0.0f,
            0.0f, cos(cameraPitch), -sin(cameraPitch),
            0.0f, sin(cameraPitch), cos(cameraPitch)
    );
    glm::mat3 yRotation = glm::mat3(
            cos(cameraYaw), 0.0f, sin(cameraYaw),
            0.0f, 1.0f, 0.0f,
            -sin(cameraYaw), 0.0f, cos(cameraYaw)
    );
    cameraRotation = yRotation * xRotation;
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
    window = glfwCreateWindow(mode->width, mode->height, "opengl raytracer", monitor,
                                          nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    screenWidth = mode->width;
    screenHeight = mode->height;

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLuint vertexShader = importAndCompileShader("../shaders/vertex.vert", GL_VERTEX_SHADER);
    GLuint fragmentShader = importAndCompileShader("../shaders/fragment.frag", GL_FRAGMENT_SHADER);

    // set up screen-size quad vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {1.0f, 1.0f, 0.0f,  // top right
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

    // link shaders
    GLuint drawCallProgram = generateProgram(vertexShader, fragmentShader);
    glUseProgram(drawCallProgram);
    GLuint outputTex;
    glGenTextures(1, &outputTex);
    glBindTexture(GL_TEXTURE_2D, outputTex);
    checkGLError("(main) glGenTextures");
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mode->width, mode->height, 0, GL_RGBA, GL_FLOAT,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindImageTexture(PIXEL_OUTPUT_BINDING, outputTex, 0, GL_FALSE, 0, GL_WRITE_ONLY,
                       GL_RGBA32F); // unit 0
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(drawCallProgram, "outputTexture"), 0);

    raytraceInit();
    // render loop
    // -----------
    int frameCount = 0;
    double startTime = glfwGetTime();
    double prevTime = startTime;
    std::cout << "BEGAN RENDER LOOP" << std::endl;
    while (!glfwWindowShouldClose(window)) {
        if (frameCount == 0) {
            glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
        }
        processInput(prevTime);
        updateCameraRotation();
        raytrace(cameraPos, cameraRotation);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glUseProgram(drawCallProgram);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(window);
        glfwPollEvents();
        frameCount++;
        checkGLError("(main) glDrawElements");
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(drawCallProgram);
    std::cout << "Average FPS: " << frameCount / (glfwGetTime() - startTime) << "\n";
    glfwTerminate();
    return 0;
}

void processInput(double& prevTime) {
    double deltaTime = glfwGetTime() - prevTime;
    prevTime += deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraRotation * glm::vec3(0.0f, 0.0f, CAMERA_MOVE_SPEED * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos += cameraRotation * glm::vec3(0.0f, 0.0f, -CAMERA_MOVE_SPEED * deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos += cameraRotation * glm::vec3(-CAMERA_MOVE_SPEED * deltaTime, 0.0f, 0.0f);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += cameraRotation * glm::vec3(CAMERA_MOVE_SPEED * deltaTime, 0.0f, 0.0f);
}
