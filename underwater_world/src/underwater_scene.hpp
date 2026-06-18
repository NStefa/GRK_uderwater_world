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

// --- Shadery ---
GLuint program;           // kolor jednolity (shader_5_1)
GLuint skyboxProgram;     // cubemapa skyboxu
GLuint texProgram;        // tekstura bez normalmapy
GLuint flowmapProgram;    // dno: kolor statyczny + normalna flow-distorted
GLuint normalFlowProgram; // obiekty 3D: kolor statyczny + normalna flow-distorted

// --- Skybox ---
GLuint skyboxVAO, skyboxVBO;
GLuint skyboxTexture;

// --- Dno (plaszczyzna) ---
GLuint planeVAO, planeVBO, planeEBO;
GLuint flowmapTexture;
GLuint sandTexture;
GLuint sandNormalTexture;

// --- Wrak (statek) ---
Core::RenderContext wreckContext;
GLuint wreckTexture;
GLuint metalNormalTexture;

// --- Skaly ---
Core::RenderContext rockContext;
GLuint rockTexture;
GLuint rockNormalTexture;

// --- Kufer ---
Core::RenderContext boxContext;
GLuint boxTexture;
GLuint boxNormalTexture;

Core::Shader_Loader shaderLoader;

// --- Kamera ---
glm::vec3 cameraPos = glm::vec3(-4.f, 0, 0);
glm::quat cameraOrientation = glm::quat(1.f, 0.f, 0.f, 0.f);
float cameraYaw = 0.f;
float cameraPitch = 0.f;

// --- Oswietlenie ---
glm::vec3 lightColor = glm::vec3(1.f, 1.f, 1.f);
glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

// --- Czas ---
float aspectRatio = 1.f;
float lastTime = -1.f;
float deltaTime = 0.f;

// --- Parametry flowmapy (sterowane klawiszami 1/2/3/4) ---
float flowSpeed = 0.15f;
float flowScale = 0.2f;

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

bool resetMouseAnchor = true;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    static double lastX = xpos;
    static double lastY = ypos;

    if (resetMouseAnchor) {
        lastX = xpos;
        lastY = ypos;
        resetMouseAnchor = false;
    }

    float xoffset = (float)(xpos - lastX);
    float yoffset = (float)(ypos - lastY);
    lastX = xpos;
    lastY = ypos;

    const float sensitivity = 0.0025f;
    cameraYaw -= xoffset * sensitivity;
    cameraPitch -= yoffset * sensitivity;

    const float pitchLimit = glm::radians(89.f);
    cameraPitch = glm::clamp(cameraPitch, -pitchLimit, pitchLimit);

    glm::quat yawQuat   = glm::angleAxis(cameraYaw,   glm::vec3(0.f, 1.f, 0.f));
    glm::quat pitchQuat = glm::angleAxis(cameraPitch, glm::vec3(1.f, 0.f, 0.f));
    cameraOrientation = yawQuat * pitchQuat;
}

glm::mat4 createCameraMatrix()
{
    glm::vec3 forward = glm::rotate(cameraOrientation, glm::vec3(0.f, 0.f, -1.f));
    glm::vec3 up      = glm::rotate(cameraOrientation, glm::vec3(0.f, 1.f,  0.f));
    return Core::createViewMatrix(cameraPos, forward, up);
}

glm::mat4 createPerspectiveMatrix()
{
    float n = 0.05f;
    float f = 100.f;
    glm::mat4 perspectiveMatrix = glm::mat4({
        1,  0,  0,  0,
        0,  aspectRatio, 0, 0,
        0,  0,  (f + n) / (n - f), 2 * f * n / (n - f),
        0,  0,  -1, 0,
    });
    return glm::transpose(perspectiveMatrix);
}

// Rysuje plaszczyzne dna: kolor statyczny, normalna flow-distorted
void drawFlowmap(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint flowMap, GLuint colorTex, GLuint normalTex, float flowMapScale = 0.05f) {
    glUseProgram(flowmapProgram);
    glm::mat4 transformation = createPerspectiveMatrix() * createCameraMatrix() * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(flowmapProgram, "transformation"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(flowmapProgram, "modelMatrix"),    1, GL_FALSE, (float*)&modelMatrix);
    glUniform3f(glGetUniformLocation(flowmapProgram, "lightDir"),    lightDir.x,   lightDir.y,   lightDir.z);
    glUniform3f(glGetUniformLocation(flowmapProgram, "lightColor"),  lightColor.x, lightColor.y, lightColor.z);
    glUniform3f(glGetUniformLocation(flowmapProgram, "cameraPos"),   cameraPos.x,  cameraPos.y,  cameraPos.z);
    glUniform1f(glGetUniformLocation(flowmapProgram, "time"),        (float)glfwGetTime());
    glUniform1f(glGetUniformLocation(flowmapProgram, "speed"),       flowSpeed);
    glUniform1f(glGetUniformLocation(flowmapProgram, "flowScale"),   flowScale);
    glUniform1f(glGetUniformLocation(flowmapProgram, "flowMapScale"), flowMapScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, flowMap);
    glUniform1i(glGetUniformLocation(flowmapProgram, "flowMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glUniform1i(glGetUniformLocation(flowmapProgram, "colorTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glUniform1i(glGetUniformLocation(flowmapProgram, "normalMap"), 2);

    Core::DrawContext(context);
    glUseProgram(0);
}

// Rysuje obiekt 3D: kolor statyczny, normalna flow-distorted (skaly, wrak, kufer)
void drawNormalFlow(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint flowMap, GLuint colorTex, GLuint normalTex, float flowMapScale = 1.0f) {
    glUseProgram(normalFlowProgram);
    glm::mat4 transformation = createPerspectiveMatrix() * createCameraMatrix() * modelMatrix;
    glUniformMatrix4fv(glGetUniformLocation(normalFlowProgram, "transformation"), 1, GL_FALSE, (float*)&transformation);
    glUniformMatrix4fv(glGetUniformLocation(normalFlowProgram, "modelMatrix"),    1, GL_FALSE, (float*)&modelMatrix);
    glUniform3f(glGetUniformLocation(normalFlowProgram, "lightDir"),    lightDir.x,   lightDir.y,   lightDir.z);
    glUniform3f(glGetUniformLocation(normalFlowProgram, "lightColor"),  lightColor.x, lightColor.y, lightColor.z);
    glUniform3f(glGetUniformLocation(normalFlowProgram, "cameraPos"),   cameraPos.x,  cameraPos.y,  cameraPos.z);
    glUniform1f(glGetUniformLocation(normalFlowProgram, "time"),        (float)glfwGetTime());
    glUniform1f(glGetUniformLocation(normalFlowProgram, "speed"),       flowSpeed);
    glUniform1f(glGetUniformLocation(normalFlowProgram, "flowScale"),   flowScale);
    glUniform1f(glGetUniformLocation(normalFlowProgram, "flowMapScale"), flowMapScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, flowMap);
    glUniform1i(glGetUniformLocation(normalFlowProgram, "flowMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glUniform1i(glGetUniformLocation(normalFlowProgram, "colorTexture"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTex);
    glUniform1i(glGetUniformLocation(normalFlowProgram, "normalMap"), 2);

    Core::DrawContext(context);
    glUseProgram(0);
}

void drawSkybox()
{
    glDepthFunc(GL_LEQUAL);
    glUseProgram(skyboxProgram);
    glm::mat4 view       = glm::mat4(glm::mat3(createCameraMatrix()));
    glm::mat4 projection = createPerspectiveMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxProgram, "view"),       1, GL_FALSE, (float*)&view);
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
    glClearColor(0.0f, 0.15f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    updateDeltaTime(glfwGetTime());

    // dno
    Core::RenderContext planeCtx;
    planeCtx.vertexArray = planeVAO;
    planeCtx.size = 6;
    drawFlowmap(planeCtx, glm::mat4(1.0f), flowmapTexture, sandTexture, sandNormalTexture);

    // kufer
    glm::mat4 boxModel = glm::translate(glm::mat4(1.0f), glm::vec3(-1.f, -1.f, -5.f));
    boxModel = glm::scale(boxModel, glm::vec3(0.02f));
    drawNormalFlow(boxContext, boxModel, flowmapTexture, boxTexture, boxNormalTexture);

    // skaly
    glm::mat4 rockModel = glm::translate(glm::mat4(1.0f), glm::vec3(3.f, -0.75f, -5.f));
    drawNormalFlow(rockContext, rockModel, flowmapTexture, rockTexture, rockNormalTexture);

    glm::mat4 rockModel2 = glm::translate(glm::mat4(1.0f), glm::vec3(-2.f, -0.65f, -7.f));
    rockModel2 = glm::rotate(rockModel2, glm::radians(45.f), glm::vec3(0.f, 1.f, 0.f));
    drawNormalFlow(rockContext, rockModel2, flowmapTexture, rockTexture, rockNormalTexture);

    glm::mat4 rockModel3 = glm::translate(glm::mat4(1.0f), glm::vec3(5.f, -0.85f, -10.f));
    drawNormalFlow(rockContext, rockModel3, flowmapTexture, rockTexture, rockNormalTexture);

    glm::mat4 rockModel4 = glm::translate(glm::mat4(1.0f), glm::vec3(-4.f, -0.69f, -12.f));
    drawNormalFlow(rockContext, rockModel4, flowmapTexture, rockTexture, rockNormalTexture);

    // wrak
    glm::mat4 wreckModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.f, -1.25f, -10.f));
    wreckModel = glm::rotate(wreckModel, glm::radians(160.f), glm::vec3(1.f, 0.f, 0.f));
    wreckModel = glm::scale(wreckModel, glm::vec3(0.007f));
    wreckModel = glm::translate(wreckModel, glm::vec3(-39.f, 0.f, -556.f));
    drawNormalFlow(wreckContext, wreckModel, flowmapTexture, wreckTexture, metalNormalTexture);

    drawSkybox();

    glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    aspectRatio = width / float(height);
    glViewport(0, 0, width, height);
}

// Pomocnicza funkcja do wczytywania modelu OBJ przez Assimp
static void loadMesh(const std::string& path, Core::RenderContext& ctx, int meshIndex = 0) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
    if (scene && (int)scene->mNumMeshes > meshIndex)
        ctx.initFromAssimpMesh(scene->mMeshes[meshIndex]);
    else
        std::cout << "Failed to load: " << path << " — " << importer.GetErrorString() << std::endl;
}

void init(GLFWwindow* window)
{
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST);

    // shadery
    program           = shaderLoader.CreateProgram("shaders/shader_5_1.vert",            "shaders/shader_5_1.frag");
    skyboxProgram     = shaderLoader.CreateProgram("shaders/shader_skybox.vert",          "shaders/shader_skybox.frag");
    texProgram        = shaderLoader.CreateProgram("shaders/shader_tex.vert",             "shaders/shader_tex.frag");
    flowmapProgram    = shaderLoader.CreateProgram("shaders/shader_flowmap.vert",         "shaders/shader_flowmap.frag");
    normalFlowProgram = shaderLoader.CreateProgram("shaders/shader_normalmap_flow.vert",  "shaders/shader_normalmap_flow.frag");

    // skybox VAO
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    skyboxTexture = Core::loadCubemap({
        "textures/skybox/px.png", "textures/skybox/nx.png",
        "textures/skybox/py.png", "textures/skybox/ny.png",
        "textures/skybox/pz.png", "textures/skybox/nz.png"
    });

    // dno: pozycja(3) + normalna(3) + uv(2) + tangent(3) + bitangent(3) = stride 14
    float planeVertices[] = {
        // pozycja              // normalna      // uv          // tangent     // bitangent
        -50.f,-1.f,-50.f,   0.f,1.f,0.f,   0.f,  0.f,   1.f,0.f,0.f,   0.f,0.f,-1.f,
         50.f,-1.f,-50.f,   0.f,1.f,0.f,   20.f, 0.f,   1.f,0.f,0.f,   0.f,0.f,-1.f,
         50.f,-1.f, 50.f,   0.f,1.f,0.f,   20.f, 20.f,  1.f,0.f,0.f,   0.f,0.f,-1.f,
        -50.f,-1.f, 50.f,   0.f,1.f,0.f,   0.f,  20.f,  1.f,0.f,0.f,   0.f,0.f,-1.f,
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
    int stride = 14 * sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3  * sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6  * sizeof(float)));
    glEnableVertexAttribArray(3); glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8  * sizeof(float)));
    glEnableVertexAttribArray(4); glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    glBindVertexArray(0);

    // modele
    loadMesh("models/ship/12219_boat_v2_L2.obj", wreckContext, 19); // mesh 19 = boat_body
    loadMesh("models/rock/sasso14.obj",           rockContext);
    loadMesh("models/box/chest_low.obj",          boxContext);

    // tekstury
    flowmapTexture    = Core::loadTexture("textures/flowmap.png");
    sandTexture       = Core::loadTexture("textures/sand/Ground080_4K-PNG_Color.png");
    sandNormalTexture = Core::loadTexture("textures/sand/Ground080_4K-PNG_NormalGL.png");
    wreckTexture      = Core::loadTexture("models/ship/boat_body_diffuse.jpg");
    metalNormalTexture= Core::loadTexture("textures/metal/Metal053C_1K-PNG_NormalGL.png");
    rockTexture       = Core::loadTexture("models/rock/sasso14.jpg");
    rockNormalTexture = Core::loadTexture("models/rock/normal.jpg");
    boxTexture        = Core::loadTexture("models/box/default_albedo.jpg");
    boxNormalTexture  = Core::loadTexture("models/box/default_normal.png");
}

void shutdown(GLFWwindow* window)
{
    shaderLoader.DeleteProgram(program);
    shaderLoader.DeleteProgram(skyboxProgram);
    shaderLoader.DeleteProgram(texProgram);
    shaderLoader.DeleteProgram(flowmapProgram);
    shaderLoader.DeleteProgram(normalFlowProgram);
}

void processInput(GLFWwindow* window)
{
    // TAB: przelacz widocznosc kursora
    static bool tabWasPressed = false;
    bool tabPressed = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabPressed && !tabWasPressed) {
        int currentMode = glfwGetInputMode(window, GLFW_CURSOR);
        bool willDisable = currentMode != GLFW_CURSOR_DISABLED;
        glfwSetInputMode(window, GLFW_CURSOR, willDisable ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (willDisable) resetMouseAnchor = true;
    }
    tabWasPressed = tabPressed;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // 1/2: predkosc flowmapy, 3/4: skala przesuniecia
    static bool key1Was = false, key2Was = false, key3Was = false, key4Was = false;
    bool key1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    bool key2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    bool key3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
    bool key4 = glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS;
    if (key1 && !key1Was) flowSpeed = glm::max(0.01f, flowSpeed - 0.05f);
    if (key2 && !key2Was) flowSpeed = glm::min(1.0f,  flowSpeed + 0.05f);
    if (key3 && !key3Was) flowScale = glm::max(0.01f, flowScale - 0.05f);
    if (key4 && !key4Was) flowScale = glm::min(1.0f,  flowScale + 0.05f);
    key1Was = key1; key2Was = key2; key3Was = key3; key4Was = key4;

    if (key1 || key2 || key3 || key4) {
        char title[128];
        snprintf(title, sizeof(title), "Underwater | speed: %.2f  flowScale: %.2f", flowSpeed, flowScale);
        glfwSetWindowTitle(window, title);
    }

    // ruch kamery (WASD + QE, SHIFT = sprint)
    glm::vec3 forward = glm::rotate(cameraOrientation, glm::vec3(0.f, 0.f, -1.f));
    glm::vec3 right   = glm::rotate(cameraOrientation, glm::vec3(1.f, 0.f,  0.f));
    glm::vec3 up      = glm::vec3(0.f, 1.f, 0.f);

    float moveSpeed = 1.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        moveSpeed *= 3.f;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += forward * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= forward * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += right   * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= right   * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += up      * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) cameraPos -= up      * moveSpeed;

    // kamera nie wychodzi ponizej podlogi
    cameraPos.y = glm::max(cameraPos.y, -1.0f + 0.2f);
}

void renderLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);
        renderScene(window);
        glfwPollEvents();
    }
}
