#ifndef PLANET_H
#define PLANET_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include "Sphere.h"

class Planet
{
private:
    float size;
    float orbitRadius;
    float orbitSpeed;
    float rotationSpeed;
    unsigned int textureID; 

public:
    Planet(float size, float orbitRadius, float orbitSpeed, float rotationSpeed, unsigned int textureID)
    {
        this->size = size;
        this->orbitRadius = orbitRadius;
        this->orbitSpeed = orbitSpeed;
        this->rotationSpeed = rotationSpeed;
        this->textureID = textureID;
    }

    glm::mat4 draw(Shader &shader, Sphere &sphere, float time, glm::mat4 parentModel = glm::mat4(1.0f))
    {
        glm::mat4 model = parentModel;

        if (orbitRadius > 0.0f)
        {
            model = glm::rotate(model, time * glm::radians(orbitSpeed), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::translate(model, glm::vec3(orbitRadius, 0.0f, 0.0f));
        }

        glm::mat4 currentOrbitContext = model;

        model = glm::rotate(model, time * glm::radians(rotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(size));
        
        shader.use();

        shader.setMat4("model", glm::value_ptr(model));
        
        glUniform1i(glGetUniformLocation(shader.ID, "useTexture"), 1); 
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(glGetUniformLocation(shader.ID, "planetTexture"), 0);

        sphere.draw();

        return currentOrbitContext;
    }
};
#endif