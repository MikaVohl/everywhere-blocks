#include "Renderer.h"
#include <glm/gtc/type_ptr.hpp>

Renderer::Renderer(const char* vertSrc, const char* fragSrc, const CubeMesh& mesh) : shader_(vertSrc, fragSrc), mesh_(mesh)
{
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
