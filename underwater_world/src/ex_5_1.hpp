#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

GLuint program;
GLuint skyboxProgram;
GLuint skyboxVAO, skyboxVBO;

GLuint texProgram;
GLuint planeVAO, planeVBO, planeEBO;
GLuint floorTexture;

GLuint skyboxTexture;

Core::Shader_Loader shaderLoader;

glm::vec3 cameraPos = glm::vec3(-4.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);
glm::vec3 lightColor = glm::vec3(1.f, 1.f, 1.f);
glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

glm::vec3 spaceshipPos = glm::vec3(-4.f, 0, 0);
glm::vec3 spaceshipDir = glm::vec3(1.f, 0.f, 0.f);

float aspectRatio = 1.f;
float lastTime = -1.f;
float deltaTime = 0.f;

float skyboxVertices[] = {
    -1.0f,  1.0f, -1.0f,   -1.0f, -1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,    1.0f,  1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,   -1.0f, -1.0f, -1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,    1.0f, -1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f,  1.0f, -1.0f,    1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,    1.0f, -1.0f,  1.0f,   -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,    1.0f,  1.0f, -1.0f,    1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   -1.0f,  1.0f,  1.0f,   -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   -1.0f, -1.0f,  1.0f,    1.0f, -1.0f,  1.0f
};

void updateDeltaTime(float time) {
    if (lastTime < 0) { lastTime = time; return; }
    deltaTime = time - lastTime;
    if (deltaTime > 0.1) deltaTime = 0.1;
    lastTime = time;
}

glm::mat4 createCameraMatrix()
{
    glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));
    glm::mat4 cameraRotrationMatrix = glm::mat4({
        cameraSide.x, cameraSide.y, cameraSide.z, 0,
        cameraUp.x, cameraUp.y, cameraUp.z, 0,
        -cameraDir.x, -cameraDir.y, -cameraDir.z, 0,
        0., 0., 0., 1.,
        });
    cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
    return cameraRotrationMatrix * glm::translate(-cameraPos);
}

glm::mat4 createPerspectiveMatrix()
{
    glm::mat4 perspectiveMatrix;
    float n = 0.05f;
    float f = 100.f;
    perspectiveMatrix = glm::mat4({
        1, 0., 0., 0.,
        0., aspectRatio, 0., 0.,
        0., 0., (f + n) / (n - f), 2 * f * n / (n - f),
        0., 0., -1., 0.,
        });
    perspectiveMatrix = glm::transpose(perspectiveMatrix);
    return perspectiveMatrix;
}

void drawObjectColor(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {
    glUseProgram(program);
    glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
    glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
    glUniform3f(glGetUniformLocation(program, "color"), color.x, color.y, color.z);
    glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
    glUniform3f(glGetUniformLocation(program, "lightColor"), lightColor.x, lightColor.y, lightColor.z);
    glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    Core::DrawContext(context);
    glUseProgram(0);
}

void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint texture) {
    glUseProgram(texProgram);
    glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
    glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(texProgram, "transformation"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(texProgram, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
    glUniform3f(glGetUniformLocation(texProgram, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
    glUniform3f(glGetUniformLocation(texProgram, "lightColor"), lightColor.x, lightColor.y, lightColor.z);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(texProgram, "colorTexture"), 0);

    Core::DrawContext(context);
    glUseProgram(0);
}

void drawSkybox()
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxProgram);
    glm::mat4 view = glm::mat4(glm::mat3(createCameraMatrix()));
    glm::mat4 projection = createPerspectiveMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"), 1, GL_FALSE, (float*)&view);
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "projection"), 1, GL_FALSE, (float*)&projection);
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
    glUniform1i(glGetUniformLocation(skyboxProgram, "skybox"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS);
    glUseProgram(0);
}

void renderScene(GLFWwindow* window)
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float time = glfwGetTime();
    updateDeltaTime(time);

    Core::RenderContext planeCtx;
    planeCtx.vertexArray = planeVAO;
    planeCtx.size = 6;
    drawObjectTexture(planeCtx, glm::mat4(1.0f), floorTexture);

    drawSkybox();

    glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    aspectRatio = width / float(height);
    glViewport(0, 0, width, height);
}

void init(GLFWwindow* window)
{
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glEnable(GL_DEPTH_TEST);

    program = shaderLoader.CreateProgram("shaders/shader_5_1.vert", "shaders/shader_5_1.frag");
    skyboxProgram = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
    texProgram = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");

    // Skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces = {
    "textures/skybox/px.png",
    "textures/skybox/nx.png",
    "textures/skybox/py.png",
    "textures/skybox/ny.png",
    "textures/skybox/pz.png",
    "textures/skybox/nz.png"
    };
    skyboxTexture = Core::loadCubemap(faces);

    float planeVertices[] = {
        // pozycja              // normal          // uv
        -2.0f, -0.5f, -2.0f,   0.f, 1.f, 0.f,    0.0f, 0.0f,
         2.0f, -0.5f, -2.0f,   0.f, 1.f, 0.f,    1.0f, 0.0f,
         2.0f, -0.5f,  2.0f,   0.f, 1.f, 0.f,    1.0f, 1.0f,
        -2.0f, -0.5f,  2.0f,   0.f, 1.f, 0.f,    0.0f, 1.0f,
    };
    unsigned int planeIndices[] = { 0,1,2,  0,2,3 };

    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);

    glBindVertexArray(planeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    int stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glBindVertexArray(0);

    floorTexture = Core::loadTexture("textures/flowmap.png");
}

void shutdown(GLFWwindow* window)
{
    shaderLoader.DeleteProgram(program);
    shaderLoader.DeleteProgram(skyboxProgram);
}

void processInput(GLFWwindow* window)
{
    glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
    glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
    float angleSpeed = 0.05f * deltaTime * 60;
    float moveSpeed = 0.05f * deltaTime * 60;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        angleSpeed *= 3;
        moveSpeed *= 3;
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        spaceshipPos += spaceshipDir * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        spaceshipPos -= spaceshipDir * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        spaceshipPos += spaceshipSide * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        spaceshipPos -= spaceshipSide * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        spaceshipPos += spaceshipUp * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        spaceshipPos -= spaceshipUp * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        spaceshipDir = glm::vec3(glm::eulerAngleY(angleSpeed) * glm::vec4(spaceshipDir, 0));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        spaceshipDir = glm::vec3(glm::eulerAngleY(-angleSpeed) * glm::vec4(spaceshipDir, 0));

    cameraPos = spaceshipPos - 0.3f * spaceshipDir + glm::vec3(0, 1, 0) * 0.1f;
    cameraDir = spaceshipDir;
}

void renderLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        renderScene(window);
        glfwPollEvents();
    }
}