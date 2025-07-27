#pragma once
#include <OpenGL/gl3.h>

class CubeMesh {
public:
    CubeMesh();
    ~CubeMesh();

    GLuint getVAO() const { return vao_; }
    GLuint getVBO() const { return vbo_; }
    GLsizei getIndexCount() const { return indexCount_; }

private:
    GLuint vao_ = 0; // Vertex Array Object
    GLuint vbo_ = 0; // Vertex Buffer Object
    GLuint ebo_ = 0; // Element Buffer Object
    GLsizei indexCount_ = 0; // Number of indices in the element buffer
};