
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    
    glm::vec3 Position;
    glm::vec3 Front;   
    glm::vec3 Up;      

    float Yaw; 
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;

    Camera(glm::vec3 position = glm::vec3(0.0f, 15.0f, 30.0f))
    {
        Position = position;
        Front = glm::vec3(0.0f, -0.4f, -1.0f);
        Up = glm::vec3(0.0f, 1.0f, 0.0f);

        Yaw = -90.0f;
        Pitch = -25.0f;

        MovementSpeed = 8.0f;
        MouseSensitivity = 0.1f;
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    void ProcessKeyboard(int direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;

        if (direction == 1)
            Position += Front * velocity;

        if (direction == 2)
            Position -= Front * velocity;

        if (direction == 3)
            Position -= glm::normalize(glm::cross(Front, Up)) * velocity;

        if (direction == 4)
            Position += glm::normalize(glm::cross(Front, Up)) * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset)
    {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;

        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));

        Front = glm::normalize(front);
    }
};

#endif