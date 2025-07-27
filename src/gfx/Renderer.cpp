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

void Renderer::setupAttributes(const CubeMesh& cube, const InstanceVBO& inst)
{
    glBindVertexArray(cube.getVAO());

    glBindBuffer(GL_ARRAY_BUFFER, cube.getVBO());
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, inst.id());
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BlockInstance), (void*)0);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_INT, sizeof(BlockInstance), (void*)offsetof(BlockInstance, texIndex));
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);
}
