#pragma once
#include "Shader.hpp"
#include "Mesh.hpp"
#include "InstanceBuffer.hpp"
#include "../world/Block.hpp"
#include <glm/glm.hpp>
#include <vector>

class Renderer {
public:
    Renderer(const char* vertSrc, const char* fragSrc, const CubeMesh& mesh);
    ~Renderer();

    void draw(const glm::mat4& vp, int instanceCount);
    ShaderProgram& shader() { return shader_; }

    void buildInstanceBuffer(const std::vector<Block>& blocks, InstanceVBO& instanceVBO);
    void setupAttributes(const CubeMesh& cube, const InstanceVBO& inst);

private:
    ShaderProgram shader_;
    const CubeMesh& mesh_;
    GLint uVP_;
    std::vector<BlockInstance> instanceBuffer_;
};
