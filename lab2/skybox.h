/*
#ifndef SKYBOX_H
#define SKYBOX_H

#include <glm/glm.hpp>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

class Skybox {
private:
    glm::vec3 position;      // Position of the building
    glm::vec3 scale;         // Scale of the building

    // OpenGL buffers and IDs
    GLuint vertexArrayID;
    GLuint vertexBufferID;
    GLuint colorBufferID;
    GLuint indexBufferID;
    GLuint uvBufferID;
    GLuint textureID;
    GLuint programID;
    GLuint mvpMatrixID;
    GLuint textureSamplerID;

    // Static buffer data
    static const GLfloat vertex_buffer_data[];
    static GLfloat color_buffer_data[];
    static const GLuint index_buffer_data[];
    static GLfloat uv_buffer_data[48];

public:
    void initializeSky(glm::vec3 position, glm::vec3 scale);
    void render(glm::mat4 vpMatrix);
    void cleanup();
};

#endif
*/