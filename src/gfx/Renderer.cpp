#include "Renderer.hpp"
#include "../world/Block.hpp"
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer(const char* vertSrc, const char* fragSrc, const CubeMesh& mesh) : shader_(vertSrc, fragSrc), mesh_(mesh) {
    uVP_ = glGetUniformLocation(shader_.id(), "uVP");
}

Renderer::~Renderer() {}

void Renderer::draw(const glm::mat4& vp, int instanceCount) {
    shader_.use();
    glUniformMatrix4fv(uVP_, 1, GL_FALSE, glm::value_ptr(vp));
    glBindVertexArray(mesh_.getVAO());
    glDrawElementsInstanced(GL_TRIANGLES, mesh_.getIndexCount(), GL_UNSIGNED_INT, 0, instanceCount);
    glBindVertexArray(0);
}

void Renderer::buildInstanceBuffer(const std::vector<Block>& blocks, InstanceVBO& instanceVBO) {
    std::vector<BlockInstance> instances;
    instances.reserve(blocks.size());
    for (const Block& block : blocks) {
        instances.push_back(BlockInstance{
            glm::vec3(block.pos), // convert ivec3 to vec3
            static_cast<int>(block.id) // assuming BlockId can be cast to int
        });
    }
    instanceVBO.update(instances.data(), instances.size());
}