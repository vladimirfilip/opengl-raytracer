#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <iostream>

#include "util.h"
#include "raytrace.h"
#include "constants.h"
#include "bvh.h"
#include "glm/gtc/type_ptr.hpp"

/*
 * Disclaimer: boilerplate to render two triangles on the screen
 * is imported from https://learnopengl.com/Getting-started/Hello-Triangle
 */

static glm::vec3 cameraPos = CAMERA_START_POS;
static glm::mat3 cameraRotation;
static double cameraPitch = 0.0f, cameraYaw = 0.0f;
static double prevMouseX, prevMouseY;
static int screenWidth, screenHeight;
static GLFWwindow* window;
static int renderMode = RENDER_MODE;
static bool clearAccumulatedFrames = false;

void processInput(double &prevTime);

static void updateCameraRotation() {
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    double dx = mouseX - prevMouseX;
    double dy = mouseY - prevMouseY;
    if (abs(dx) > 0.001f || abs(dy) > 0.001f) {
        clearAccumulatedFrames = true;
    }
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
    checkGLError("(main) prior to drawProgram init");
    // link shaders
    GLuint drawProgram = generateProgram(vertexShader, fragmentShader);
    checkGLError("(main) generateProgram()");
    GLint success = 0;
    glGetProgramiv(drawProgram, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        // Linking failed, you can get the info log for details:
        GLint logLength = 0;
        glGetProgramiv(drawProgram, GL_INFO_LOG_LENGTH, &logLength);

        std::vector<GLchar> infoLog(logLength);
        glGetProgramInfoLog(drawProgram, logLength, NULL, infoLog.data());

        // Print or log the error
        std::cerr << "Shader drawProgram linking failed:\n" << infoLog.data() << std::endl;
    }
    glUseProgram(drawProgram);
    GLuint currentFrame, prevFrame;
    glGenTextures(1, &currentFrame);
    glBindTexture(GL_TEXTURE_2D, currentFrame);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, screenWidth, screenHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glGenTextures(1, &prevFrame);
    glBindTexture(GL_TEXTURE_2D, prevFrame);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, screenWidth, screenHeight);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(glGetUniformLocation(drawProgram, "outputTexture"), 0);

    GLuint raytraceProgram = raytraceInit();
    glUseProgram(raytraceProgram);
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ScreenWidth"),
                static_cast<GLfloat>(screenWidth));
    checkGLError("(raytraceInit) set screenWidth");
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ScreenHeight"),
                static_cast<GLfloat>(screenHeight));
    checkGLError("(raytraceInit) set screenHeight");
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_FOV"),
                static_cast<GLfloat>(FOV * std::numbers::pi / 180.f));
    checkGLError("(raytraceInit) set FOV");
    glUniform1f(glGetUniformLocation(raytraceProgram, "u_ViewportDist"), VIEWPORT_DIST);
    checkGLError("(raytraceInit) set viewportDist");
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RaysPerPixel"), RAYS_PER_PIXEL);
    glUniform1ui(glGetUniformLocation(raytraceProgram, "u_RayBounces"), RAY_BOUNCES);
    checkGLError("(raytraceInit) set uniforms");
    checkGLError("(main) after raytraceInit()");
    // render loop
    // -----------
    int frameCount = 0, totalFrames = 0;
    double startTime = glfwGetTime();
    double prevTime = startTime;
    std::cout << "BEGAN RENDER LOOP" << std::endl;
    glUseProgram(drawProgram);
    glfwGetCursorPos(window, &prevMouseX, &prevMouseY);
    const int num_groups_x = (screenWidth + RAYTRACE_WORKGROUP_SIZE - 1) / RAYTRACE_WORKGROUP_SIZE;
    const int num_groups_y = (screenHeight + RAYTRACE_WORKGROUP_SIZE - 1) / RAYTRACE_WORKGROUP_SIZE;
    while (!glfwWindowShouldClose(window)) {
        clearAccumulatedFrames = false;
        processInput(prevTime);
        updateCameraRotation();
        if (clearAccumulatedFrames)
            frameCount = 0;
        glBindImageTexture(CURR_FRAME_BINDING, currentFrame, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(PREV_FRAME_BINDING, prevFrame, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        raytrace(cameraPos, cameraRotation, renderMode, frameCount, num_groups_x, num_groups_y);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        glBindTexture(GL_TEXTURE_2D, currentFrame);
        glUseProgram(drawProgram);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glfwSwapBuffers(window);
        std::swap(currentFrame, prevFrame);
        glfwPollEvents();
        frameCount++;
        totalFrames++;
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(drawProgram);
    std::cout << "Average FPS: " << totalFrames / (glfwGetTime() - startTime) << "\n";
    glfwTerminate();
    return 0;
}

void processInput(double& prevTime) {
    double deltaTime = glfwGetTime() - prevTime;
    prevTime += deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        glfwSetWindowShouldClose(window, true);
    }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        cameraPos += cameraRotation * glm::vec3(0.0f, 0.0f, CAMERA_MOVE_SPEED * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        cameraPos += cameraRotation * glm::vec3(0.0f, 0.0f, -CAMERA_MOVE_SPEED * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        cameraPos += cameraRotation * glm::vec3(-CAMERA_MOVE_SPEED * deltaTime, 0.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        cameraPos += cameraRotation * glm::vec3(CAMERA_MOVE_SPEED * deltaTime, 0.0f, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        renderMode = RENDER_MODE;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        renderMode = TRIANGLE_TEST_MODE;
    }
    if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        clearAccumulatedFrames = true;
        renderMode = BOX_TEST_MODE;
    }
}
