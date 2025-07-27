#include "Mesh.hpp"
#include <OpenGL/gl3.h>

CubeMesh::CubeMesh() {
    float verts[] = {
        // Front face (z = +0.5)
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, // 0
        0.5f, -0.5f,  0.5f, 1.0f, 0.0f, // 1
        0.5f,  0.5f,  0.5f, 1.0f, 1.0f, // 2
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, // 3

        // Right face (x = +0.5)
        0.5f, -0.5f,  0.5f, 0.0f, 0.0f, // 4
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // 5
        0.5f,  0.5f, -0.5f, 1.0f, 1.0f, // 6
        0.5f,  0.5f,  0.5f, 0.0f, 1.0f, // 7

        // Back face (z = -0.5)
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // 8
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // 9
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, // 10
        0.5f,  0.5f, -0.5f, 0.0f, 1.0f, // 11

        // Left face (x = -0.5)
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // 12
        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, // 13
        -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, // 14
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, // 15

        // Top face (y = +0.5)
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, // 16
        0.5f,  0.5f,  0.5f, 1.0f, 0.0f, // 17
        0.5f,  0.5f, -0.5f, 1.0f, 1.0f, // 18
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, // 19

        // Bottom face (y = -0.5)
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // 20
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // 21
        0.5f, -0.5f,  0.5f, 1.0f, 1.0f, // 22
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, // 23
    };
    unsigned idx[] = {
        // Front face
        0, 1, 2,  2, 3, 0,
        // Right face
        4, 5, 6,  6, 7, 4,
        // Back face
        8, 9,10, 10,11, 8,
        // Left face
        12,13,14, 14,15,12,
        // Top face
        16,17,18, 18,19,16,
        // Bottom face
        20,21,22, 22,23,20
    };

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), (void*)0);
    indexCount_ = sizeof(idx)/sizeof(idx[0]);
    glBindVertexArray(0);
}

CubeMesh::~CubeMesh() {
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
}