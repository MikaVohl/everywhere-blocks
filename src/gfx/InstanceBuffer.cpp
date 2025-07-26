#include "InstanceBuffer.h"
#include <OpenGL/gl3.h>

InstanceVBO::InstanceVBO() {
    glGenBuffers(1, &vbo_);
}
InstanceVBO::~InstanceVBO() {
    glDeleteBuffers(1, &vbo_);
}
void InstanceVBO::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
}
void InstanceVBO::update(const BlockInstance* blocks, size_t count) {
    bind();
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(BlockInstance), blocks, GL_STATIC_DRAW);
}