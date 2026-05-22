#ifndef RING_H
#define RING_H

#include <GL/glew.h>
#include <vector>
#include <cmath>

class Ring
{
public:
    unsigned int VAO, VBO;
    int vertexCount;

    Ring(float innerRadius, float outerRadius, int ringSegments = 256)
    {
        std::vector<float> vertices;

        for (int i = 0; i <= ringSegments; ++i)
        {
            float angle = ((float)i / ringSegments) * 2.0f * 3.14159265359f;
            float c = cos(angle);
            float s = sin(angle);

            float innerU = c * (innerRadius / outerRadius) * 0.5f + 0.5f;
            float innerV = s * (innerRadius / outerRadius) * 0.5f + 0.5f;

            float outerU = c * 0.5f + 0.5f;
            float outerV = s * 0.5f + 0.5f;

            vertices.push_back(innerRadius * c);
            vertices.push_back(0.0f);
            vertices.push_back(innerRadius * s);
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            vertices.push_back(innerU);
            vertices.push_back(innerV);

            vertices.push_back(outerRadius * c);
            vertices.push_back(0.0f);
            vertices.push_back(outerRadius * s);
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
            vertices.push_back(outerU);
            vertices.push_back(outerV);
        }

        vertexCount = vertices.size() / 8;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        GLsizei stride = 8 * sizeof(float);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));

        glBindVertexArray(0);
    }

    void draw()
    {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, vertexCount);
        glBindVertexArray(0);
    }

    ~Ring()
    {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }
};

#endif