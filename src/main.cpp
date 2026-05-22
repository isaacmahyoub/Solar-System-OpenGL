#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"
#include "Sphere.h"
#include "Camera.h"
#include "Planet.h"
#include "Ring.h"

const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

Camera camera;
bool firstMouse = true;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

unsigned int loadTexture(const char *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;

    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if (nrComponents == 4)
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);

    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

void drawOrbit(float radius, Shader &shader)
{
    glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 0);
    const int segments = 100;
    std::vector<float> orbitVertices;
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * 3.14159f * i / segments;
        orbitVertices.push_back(cos(angle) * radius);
        orbitVertices.push_back(0.0f);
        orbitVertices.push_back(sin(angle) * radius);
    }

    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, orbitVertices.size() * sizeof(float), orbitVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    shader.setVec3("objectColor", 0.3f, 0.3f, 0.3f);
    shader.setVec3("emission", 1.0f, 1.0f, 1.0f);
    glDrawArrays(GL_LINE_LOOP, 0, segments);

    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Solar System with Textures & Bloom Trick", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glewInit() != GLEW_OK)
        return -1;
    glEnable(GL_DEPTH_TEST);

    Shader spaceShader("src/vertex_shader.glsl", "src/fragment_shader.glsl");
    Sphere sphereShape(1.0f, 36, 18);

    unsigned int sunTex = loadTexture("textures/sun.jpg");
    unsigned int earthTex = loadTexture("textures/earth.jpg");
    unsigned int earthSpecularTex = loadTexture("textures/earth_specular.jpg");
    unsigned int moonTex = loadTexture("textures/moon.jpg");
    unsigned int marsTex = loadTexture("textures/mars.jpg");
    unsigned int jupiterTex = loadTexture("textures/jupiter.jpg");
    unsigned int saturnTex = loadTexture("textures/saturn.jpg");
    unsigned int mercuryTex = loadTexture("textures/mercury.jpg");
    unsigned int venusSurfaceTex = loadTexture("textures/venus_surface.jpg");
    unsigned int venusAtmosphereTex = loadTexture("textures/venus_atmosphere.jpg");
    unsigned int uranusTex = loadTexture("textures/uranus.jpg");
    unsigned int neptuneTex = loadTexture("textures/neptune.jpg");

    unsigned int saturnRingTexture = loadTexture("textures/saturn_ring.png");

    Planet sun(2.2f, 0.0f, 0.0f, 15.0f, sunTex);
    Planet mercury(0.25f, 3.8f, 10.0f, 150.0f, mercuryTex);

    Planet venusSurface(0.55f, 5.5f, -5.0f, 120.0f, venusSurfaceTex);
    Planet venusAtmosphere(0.565f, 5.5f, -5.0f, 120.0f, venusAtmosphereTex);

    Planet earth(0.6f, 7.5f, 35.0f, 100.0f, earthTex);
    Planet moon(0.18f, 1.5f, 180.0f, 40.0f, moonTex);
    Planet mars(0.45f, 10.5f, 20.0f, 90.0f, marsTex);
    Planet jupiter(1.2f, 16.0f, 45.0f, 60.0f, jupiterTex);
    Planet saturn(0.85f, 21.0f, 40.0f, 45.0f, saturnTex);
    Planet uranus(0.65f, 25.0f, -30.0f, 30.0f, uranusTex);
    Planet neptune(0.62f, 28.5f, 35.0f, 20.0f, neptuneTex);

    float skyboxVertices[] = {
        -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    Shader skyboxShader("src/skybox_vertex.glsl", "src/skybox_fragment.glsl");

    std::vector<std::string> faces{
        "textures/skybox/right.png",
        "textures/skybox/left.png",
        "textures/skybox/top.png",
        "textures/skybox/bottom.png",
        "textures/skybox/front.png",
        "textures/skybox/back.png"};
    unsigned int cubemapTexture = loadCubemap(faces);

    skyboxShader.use();
    glUniform1i(glGetUniformLocation(skyboxShader.ID, "skybox"), 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Ring saturnRing(1.0f, 2.2f);

    unsigned int asteroidAmount = 1000;
    glm::mat4 *modelMatrices = new glm::mat4[asteroidAmount];

    float beltRadius = 13.0f;
    float beltOffset = 0.6f;

    for (unsigned int i = 0; i < asteroidAmount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);

        float angle = (float)i / (float)asteroidAmount * 360.0f;
        float displacement = ((rand() % (int)(2 * beltOffset * 100)) / 100.0f) - beltOffset;
        float x = sin(angle) * beltRadius + displacement;

        displacement = ((rand() % (int)(2 * beltOffset * 100)) / 100.0f) - beltOffset;
        float y = displacement * 0.3f;

        displacement = ((rand() % (int)(2 * beltOffset * 100)) / 100.0f) - beltOffset;
        float z = cos(angle) * beltRadius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        float scale = (rand() % 20) / 300.0f + 0.02f;
        model = glm::scale(model, glm::vec3(scale));

        float rotAngle = (float)(rand() % 360);
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        modelMatrices[i] = model;
    }

    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, asteroidAmount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    glBindVertexArray(sphereShape.getVAO());

    GLsizei vec4Size = sizeof(glm::vec4);
    for (unsigned int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(uintptr_t)(i * vec4Size));
        glVertexAttribDivisor(3 + i, 1);
    }

    glBindVertexArray(0);
    delete[] modelMatrices;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.005f, 0.005f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        spaceShader.use();

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        spaceShader.setMat4("projection", glm::value_ptr(projection));
        spaceShader.setMat4("view", glm::value_ptr(camera.GetViewMatrix()));
        spaceShader.setVec3("lightPos", 0.0f, 0.0f, 0.0f);
        spaceShader.setVec3("viewPos", camera.Position.x, camera.Position.y, camera.Position.z);

        spaceShader.setVec3("emission", 1.0f, 1.0f, 1.0f);
        sun.draw(spaceShader, sphereShape, currentFrame);

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDepthMask(GL_FALSE);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 0);

        glm::mat4 glowModel1 = glm::scale(glm::mat4(1.0f), glm::vec3(2.4f));
        spaceShader.setMat4("model", glm::value_ptr(glowModel1));
        spaceShader.setVec3("objectColor", 0.2f, 0.12f, 0.04f);
        sphereShape.draw();

        glm::mat4 glowModel2 = glm::scale(glm::mat4(1.0f), glm::vec3(2.8f));
        spaceShader.setMat4("model", glm::value_ptr(glowModel2));
        spaceShader.setVec3("objectColor", 0.08f, 0.04f, 0.01f);
        sphereShape.draw();

        glDepthMask(GL_TRUE);

        drawOrbit(3.8f, spaceShader);
        drawOrbit(5.5f, spaceShader);
        drawOrbit(7.5f, spaceShader);
        drawOrbit(10.5f, spaceShader);
        drawOrbit(16.0f, spaceShader);
        drawOrbit(21.0f, spaceShader);
        drawOrbit(25.0f, spaceShader);
        drawOrbit(28.5f, spaceShader);

        spaceShader.setVec3("emission", 0.0f, 0.0f, 0.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 1);

        mercury.draw(spaceShader, sphereShape, currentFrame);

        venusSurface.draw(spaceShader, sphereShape, currentFrame);

        glDepthMask(GL_FALSE);
        venusAtmosphere.draw(spaceShader, sphereShape, currentFrame);
        glDepthMask(GL_TRUE);

        spaceShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, earthTex);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "texture_diffuse1"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, earthSpecularTex);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "texture_specular1"), 1);

        glUniform1i(glGetUniformLocation(spaceShader.ID, "useSpecularMap"), 1);
        glm::mat4 earthOrbit = earth.draw(spaceShader, sphereShape, currentFrame);

        glUniform1i(glGetUniformLocation(spaceShader.ID, "useSpecularMap"), 0);

        moon.draw(spaceShader, sphereShape, currentFrame, earthOrbit);
        mars.draw(spaceShader, sphereShape, currentFrame);
        jupiter.draw(spaceShader, sphereShape, currentFrame);

        glm::mat4 saturnOrbit = saturn.draw(spaceShader, sphereShape, currentFrame);

        uranus.draw(spaceShader, sphereShape, currentFrame);
        neptune.draw(spaceShader, sphereShape, currentFrame);

        spaceShader.use();
        glUniform1i(glGetUniformLocation(spaceShader.ID, "isInstanced"), 1);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 0);
        spaceShader.setVec3("objectColor", 0.45f, 0.42f, 0.40f);

        glBindVertexArray(sphereShape.getVAO());
        glDrawElementsInstanced(GL_TRIANGLES, sphereShape.getIndexCount(), GL_UNSIGNED_INT, 0, asteroidAmount);
        glBindVertexArray(0);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "isInstanced"), 0);

        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();

        glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", glm::value_ptr(skyboxView));
        skyboxShader.setMat4("projection", glm::value_ptr(projection));

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);

        spaceShader.use();
        spaceShader.setVec3("emission", 1.0f, 1.0f, 1.0f);

        glm::mat4 ringModel = saturnOrbit;
        ringModel = glm::rotate(ringModel, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        spaceShader.setMat4("model", glm::value_ptr(ringModel));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, saturnRingTexture);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 1);

        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        saturnRing.draw();
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);

        spaceShader.setVec3("emission", 0.0f, 0.0f, 0.0f);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(1, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(2, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(3, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(4, deltaTime);
}

void mouse_callback(GLFWwindow *window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}