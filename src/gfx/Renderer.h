#pragma once
#include "Shader.h"
#include "Mesh.h"
#include <glm/glm.hpp>

class Renderer {
public:
    Renderer(const char* vertSrc, const char* fragSrc, const CubeMesh& mesh);
    ~Renderer();

    void draw(const glm::mat4& vp, int instanceCount);
    ShaderProgram& shader() { return shader_; }

private:
    ShaderProgram shader_;
    const CubeMesh& mesh_;
    GLint uVP_;
};
