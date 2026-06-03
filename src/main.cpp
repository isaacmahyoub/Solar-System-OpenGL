#include <GL/glew.h>    // OpenGL Extension Wrangler library
#include <GLFW/glfw3.h> // Window and input management library
#include <iostream>
#include <vector>
#include <cmath>

// Enable stb_image implementation for loading textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "Shader.h"
#include "Sphere.h"
#include "Camera.h"
#include "Planet.h"
#include "Ring.h"

// Global Settings & Variables
const unsigned int SCR_WIDTH = 1200; // Window width
const unsigned int SCR_HEIGHT = 800; // Window height

Camera camera;                   // Camera object for view control
bool firstMouse = true;          // Flag to handle initial mouse jump
float lastX = SCR_WIDTH / 2.0f;  // Last recorded X position of mouse (centered)
float lastY = SCR_HEIGHT / 2.0f; // Last recorded Y position of mouse (centered)
float deltaTime = 0.0f;          // Time between current frame and last frame
float lastFrame = 0.0f;          // Time of last frame

// Forward Declarations
void processInput(GLFWwindow *window);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);

// Texture Loading Function (2D Textures)
unsigned int loadTexture(const char *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID); // Generate texture ID

    int width, height, nrComponents;
    // Flip textures vertically as OpenGL expects the Y-axis to start from the bottom
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0); // Load image data from file path

    if (data)
    {
        GLenum format = GL_RGB;
        // Assign format based on image channels
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA; // Support transparency

        glBindTexture(GL_TEXTURE_2D, textureID);
        // Upload image data to GPU memory
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps for optimized minification filtering and performance at various distances

        // Texture Wrapping settings
        if (nrComponents == 4)
        {
            // Use CLAMP_TO_EDGE for transparent textures to prevent artifacts at edges
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        else
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        // Texture Filtering settings for smooth scaling
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data); // Free RAM after uploading to GPU
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

// Cubemap Loading Function (Skybox for space background)
unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    // Do not flip cubemap faces vertically as they are pre-aligned
    stbi_set_flip_vertically_on_load(false);

    // Load the 6 faces of the cubemap (Right, Left, Top, Bottom, Front, Back)
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            // Bind each image to its corresponding cubemap face target
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    // Cubemap filtering and wrapping configurations
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// Function to Draw Orbital Lines
void drawOrbit(float radius, Shader &shader)
{
    glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 0); // Disable texture mapping for solid color lines
    const int segments = 100;                                      // Resolution of the circular orbit
    std::vector<float> orbitVertices;

    // Calculate circular points on the XZ plane using trigonometry
    for (int i = 0; i <= segments; i++)
    {
        float angle = 2.0f * 3.14159f * i / segments;
        orbitVertices.push_back(cos(angle) * radius); // X coordinate
        orbitVertices.push_back(0.0f);                // Y coordinate (Flat plane)
        orbitVertices.push_back(sin(angle) * radius); // Z coordinate
    }

    // Generate VAO and VBO to pass orbit data to the GPU
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, orbitVertices.size() * sizeof(float), orbitVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // Set orbit color (dim gray) and emission strength
    shader.setVec3("objectColor", 0.3f, 0.3f, 0.3f);
    shader.setVec3("emission", 1.0f, 1.0f, 1.0f);

    // Render the line loop
    glDrawArrays(GL_LINE_LOOP, 0, segments);

    // Clean up temporary buffers
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}

// Main Application Entry Point
int main()
{
    // 1. Initialize GLFW and configure OpenGL context settings (3.3 Core Profile)
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Create the Window context
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "3D Solar System", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Configure input behavior: Capture mouse and hide cursor
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 3. Initialize GLEW to access modern OpenGL functions
    if (glewInit() != GLEW_OK)
        return -1;

    // Enable Depth Testing to ensure correct spatial rendering hierarchy
    glEnable(GL_DEPTH_TEST);

    // 4. Compile Shaders and define base Geometric models
    Shader spaceShader("src/vertex_shader.glsl", "src/fragment_shader.glsl");
    Sphere sphereShape(1.0f, 36, 18); // Mathematical sphere template shared by celestial bodies

    // 5. Load celestial body textures
    unsigned int sunTex = loadTexture("textures/sun.jpg");
    unsigned int mercuryTex = loadTexture("textures/mercury.jpg");
    unsigned int venusSurfaceTex = loadTexture("textures/venus_surface.jpg");
    unsigned int venusAtmosphereTex = loadTexture("textures/venus_atmosphere.jpg");
    unsigned int earthTex = loadTexture("textures/earth.jpg");
    unsigned int earthSpecularTex = loadTexture("textures/earth_specular.jpg"); // Specular map for oceans
    unsigned int moonTex = loadTexture("textures/moon.jpg");
    unsigned int marsTex = loadTexture("textures/mars.jpg");
    unsigned int jupiterTex = loadTexture("textures/jupiter.jpg");
    unsigned int saturnTex = loadTexture("textures/saturn.jpg");
    unsigned int saturnRingTexture = loadTexture("textures/saturn_ring.png"); // Transparent ring texture
    unsigned int uranusTex = loadTexture("textures/uranus.jpg");
    unsigned int neptuneTex = loadTexture("textures/neptune.jpg");

    // 6. Define Planets instances (Radius, Orbit Distance, Phase Offset, Rotation Speed, Texture ID)
    Planet sun(2.2f, 0.0f, 0.0f, 15.0f, sunTex);
    Planet mercury(0.25f, 3.8f, 10.0f, 150.0f, mercuryTex);
    Planet venusSurface(0.55f, 5.5f, -5.0f, 120.0f, venusSurfaceTex);
    Planet venusAtmosphere(0.565f, 5.5f, -5.0f, 120.0f, venusAtmosphereTex); // Slightly larger than surface
    Planet earth(0.6f, 7.5f, 35.0f, 100.0f, earthTex);
    Planet moon(0.18f, 1.5f, 180.0f, 40.0f, moonTex); // Orbiting relative to Earth
    Planet mars(0.45f, 10.5f, 20.0f, 90.0f, marsTex);
    Planet jupiter(1.2f, 16.0f, 45.0f, 60.0f, jupiterTex);
    Planet saturn(0.85f, 21.0f, 40.0f, 45.0f, saturnTex);
    Planet uranus(0.65f, 25.0f, -30.0f, 30.0f, uranusTex);
    Planet neptune(0.62f, 28.5f, 35.0f, 20.0f, neptuneTex);

    // 7. Setup Skybox Background
    float skyboxVertices[] = {
        // Coordinate points for rendering the internal faces of the environment cube
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
        "textures/skybox/right.png", "textures/skybox/left.png",
        "textures/skybox/top.png", "textures/skybox/bottom.png",
        "textures/skybox/front.png", "textures/skybox/back.png"};
    unsigned int cubemapTexture = loadCubemap(faces);
    skyboxShader.use();
    glUniform1i(glGetUniformLocation(skyboxShader.ID, "skybox"), 0);

    // Enable Alpha Blending for rendering transparent textures smoothly
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Generate Saturn's Ring component
    Ring saturnRing(1.0f, 2.2f);

    // 8. Generate Asteroid Belt data utilizing Instanced Rendering (Batch rendering performance boost)
    unsigned int asteroidAmount = 1000; // Total count of floating asteroids
    glm::mat4 *modelMatrices = new glm::mat4[asteroidAmount];

    float beltRadius = 13.0f; // Target distribution radius (between Mars & Jupiter)
    float beltOffset = 0.6f;  // Scale factor for random offset displacement

    for (unsigned int i = 0; i < asteroidAmount; i++)
    {
        glm::mat4 model = glm::mat4(1.0f);

        // Distribute asteroids around a circular path with randomized displacements
        float angle = (float)i / (float)asteroidAmount * 360.0f;
        float displacement = ((rand() % (int)(2 * beltOffset * 100)) / 100.0f) - beltOffset;
        float x = sin(angle) * beltRadius + displacement;

        displacement = ((rand() % (int)(2 * beltOffset * 100)) / 100.0f) - beltOffset;
        float y = displacement * 0.3f; // Squash height profile to make it a flat belt

        displacement = ((rand() % (int)(2 * beltOffset * 100)) / 100.0f) - beltOffset;
        float z = cos(angle) * beltRadius + displacement;
        model = glm::translate(model, glm::vec3(x, y, z));

        // Random scaling to give unique sizing to each rock
        float scale = (rand() % 20) / 300.0f + 0.02f;
        model = glm::scale(model, glm::vec3(scale));

        // Assign a random rotation axis and rotation offset angle
        float rotAngle = (float)(rand() % 360);
        model = glm::rotate(model, rotAngle, glm::vec3(0.4f, 0.6f, 0.8f));

        modelMatrices[i] = model; // Preserve transformation matrices array
    }

    // Allocate Instance VBO buffer to push 1000 matrices to the GPU VRAM at once
    unsigned int instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, asteroidAmount * sizeof(glm::mat4), &modelMatrices[0], GL_STATIC_DRAW);

    glBindVertexArray(sphereShape.getVAO());
    GLsizei vec4Size = sizeof(glm::vec4);
    // A mat4 structure consumes 4 vertex attribute locations (Locations: 3, 4, 5, 6)
    for (unsigned int i = 0; i < 4; i++)
    {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void *)(uintptr_t)(i * vec4Size));
        glVertexAttribDivisor(3 + i, 1); // Tell OpenGL to cycle the index attribute per-instance, not per-vertex
    }
    glBindVertexArray(0);
    delete[] modelMatrices; // Deallocate CPU memory since data is safely transferred to VRAM

    // Core Application Rendering Cycle (Render Loop)
    while (!glfwWindowShouldClose(window))
    {
        // Calculate Delta Time to normalize animation velocity across various CPU/GPU speeds
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window); // Manage keyboard interactions

        // Clear display color buffer (dark space blue tone) and depth buffers
        glClearColor(0.005f, 0.005f, 0.01f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        spaceShader.use();

        // Calculate 3D Projection Perspective and View tracking configurations
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        spaceShader.setMat4("projection", glm::value_ptr(projection));
        spaceShader.setMat4("view", glm::value_ptr(camera.GetViewMatrix()));
        spaceShader.setVec3("lightPos", 0.0f, 0.0f, 0.0f); // The Sun serves as our central point light source (0,0,0)
        spaceShader.setVec3("viewPos", camera.Position.x, camera.Position.y, camera.Position.z);

        // A) Draw Sun (Emissive self-luminous object)
        spaceShader.setVec3("emission", 1.0f, 1.0f, 1.0f);
        sun.draw(spaceShader, sphereShape, currentFrame);

        // B) Simulate Glow Visual effect (Fake Bloom Trick)
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE); // Additive blending mode for overlapping highlights
        glDepthMask(GL_FALSE);       // Disable writing to the depth buffer so glow doesn't occlude planets behind it
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 0);

        // Inner golden-orange glow layer
        glm::mat4 glowModel1 = glm::scale(glm::mat4(1.0f), glm::vec3(2.4f));
        spaceShader.setMat4("model", glm::value_ptr(glowModel1));
        spaceShader.setVec3("objectColor", 0.2f, 0.12f, 0.04f);
        sphereShape.draw();

        // Outer fainter glow layer
        glm::mat4 glowModel2 = glm::scale(glm::mat4(1.0f), glm::vec3(2.8f));
        spaceShader.setMat4("model", glm::value_ptr(glowModel2));
        spaceShader.setVec3("objectColor", 0.08f, 0.04f, 0.01f);
        sphereShape.draw();

        glDepthMask(GL_TRUE); // Re-enable writing to depth buffer

        // C) Draw orbital trails
        drawOrbit(3.8f, spaceShader);
        drawOrbit(5.5f, spaceShader);
        drawOrbit(7.5f, spaceShader);
        drawOrbit(10.5f, spaceShader);
        drawOrbit(16.0f, spaceShader);
        drawOrbit(21.0f, spaceShader);
        drawOrbit(25.0f, spaceShader);
        drawOrbit(28.5f, spaceShader);

        // Reset shading parameters back to non-emissive states for rendering standard planets
        spaceShader.setVec3("emission", 0.0f, 0.0f, 0.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 1);

        // D) Render inner planets
        mercury.draw(spaceShader, sphereShape, currentFrame);
        venusSurface.draw(spaceShader, sphereShape, currentFrame);

        // Render transparent atmosphere overlay for Venus
        glDepthMask(GL_FALSE);
        venusAtmosphere.draw(spaceShader, sphereShape, currentFrame);
        glDepthMask(GL_TRUE);

        // E) Render Earth and its natural satellite (Using custom Specular Maps)
        spaceShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, earthTex);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "texture_diffuse1"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, earthSpecularTex);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "texture_specular1"), 1);

        glUniform1i(glGetUniformLocation(spaceShader.ID, "useSpecularMap"), 1); // Activate specular ocean reflections
        glm::mat4 earthOrbit = earth.draw(spaceShader, sphereShape, currentFrame);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useSpecularMap"), 0); // Disable for remaining elements

        // Pass the calculated earthOrbit matrix to draw the Moon relative to Earth's transform space
        moon.draw(spaceShader, sphereShape, currentFrame, earthOrbit);

        // F) Render remaining outer planets
        mars.draw(spaceShader, sphereShape, currentFrame);
        jupiter.draw(spaceShader, sphereShape, currentFrame);
        glm::mat4 saturnOrbit = saturn.draw(spaceShader, sphereShape, currentFrame); // Store matrix for ring attachments
        uranus.draw(spaceShader, sphereShape, currentFrame);
        neptune.draw(spaceShader, sphereShape, currentFrame);

        // G) Draw Asteroid Belt utilizing Instanced rendering loop configurations
        spaceShader.use();
        glUniform1i(glGetUniformLocation(spaceShader.ID, "isInstanced"), 1); // Activate instance switch in Shader
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 0);  // Asteroids render using a basic solid gray rock color
        spaceShader.setVec3("objectColor", 0.45f, 0.42f, 0.40f);

        glBindVertexArray(sphereShape.getVAO());
        glDrawElementsInstanced(GL_TRIANGLES, sphereShape.getIndexCount(), GL_UNSIGNED_INT, 0, asteroidAmount);
        glBindVertexArray(0);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "isInstanced"), 0); // Deactivate instance switch

        // H) Draw Background Skybox (Rendered last for performance optimization via early depth culling)
        glDepthFunc(GL_LEQUAL); // Modify depth function parameters to let the background pass at max depth (1.0)
        skyboxShader.use();

        // Strip translation components out of view matrix so background appears infinitely distant
        glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyboxShader.setMat4("view", glm::value_ptr(skyboxView));
        skyboxShader.setMat4("projection", glm::value_ptr(projection));

        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS); // Restore depth test configurations back to default behaviors

        // I) Draw planetary ring systems (Saturn's Ring)
        spaceShader.use();
        spaceShader.setVec3("emission", 1.0f, 1.0f, 1.0f); // Brighten ring presence cleanly

        // Inherit Saturn's translation space transformations and apply a tilted angle offset (20 degrees)
        glm::mat4 ringModel = saturnOrbit;
        ringModel = glm::rotate(ringModel, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        spaceShader.setMat4("model", glm::value_ptr(ringModel));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, saturnRingTexture);
        glUniform1i(glGetUniformLocation(spaceShader.ID, "useTexture"), 1);

        glDepthMask(GL_FALSE);   // Temporarily turn off depth writing for transparent blending cycles
        glDisable(GL_CULL_FACE); // Disable backface culling to ensure rings render from above and below
        saturnRing.draw();
        glEnable(GL_CULL_FACE); // Re-enable face culling for optimization
        glDepthMask(GL_TRUE);

        spaceShader.setVec3("emission", 0.0f, 0.0f, 0.0f);

        // Swap buffers to push freshly completed frame data onto display panels and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Terminate window and clear allocated framework contexts before exiting application execution
    glfwTerminate();
    return 0;
}

// Input Event Monitoring Handler (Keyboard interactions: WASD)
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true); // Shutdown context on Escape key

    // Pass target direction and current delta timing variables to preserve frame rate independent kinetics
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(1, deltaTime); // Move Forward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(2, deltaTime); // Move Backward
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(3, deltaTime); // Move Left
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(4, deltaTime); // Move Right
}

// Mouse Track Movement Callback Handler (Look around controls)
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

    // Evaluate movement delta shifts relative to previous frame records
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Inverted since screen Y coordinates descend downwards

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset); // Pass calculated spatial drift to update rotation vectors
}
