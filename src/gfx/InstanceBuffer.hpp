#pragma once // Header guard to prevent multiple inclusions
#include <OpenGL/gl3.h>
#include <glm/glm.hpp>
#include <cstddef> // offsetof

struct BlockInstance { // Intended for GPU instancing
    glm::vec3 pos; // location 1
    int texIndex; // location 3
};

static_assert(sizeof(BlockInstance) == 16, "BlockInstance must stay tightly packed (vec3+int = 16 B)");

class InstanceVBO {
public:
    InstanceVBO();
    ~InstanceVBO();

    void bind() const;
    GLuint id() const { return vbo_; }

    void update(const BlockInstance* blocks, size_t count);

private:
    GLuint vbo_ = 0;
};
