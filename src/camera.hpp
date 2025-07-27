#pragma once
#include <glm/glm.hpp>

struct Camera {
    glm::vec3 pos{0.0f, 0.0f, 3.0f};
    float yaw   = -90.0f; // degrees
    float pitch =   0.0f; // degrees
    float fov   =  70.0f; // degrees
    float nearp =   0.1f;
    float farp  = 1000.0f;
    float mouseSensitivity = 0.1f;
    float moveSpeed = 5.0f;

    glm::mat4 view() const;
    glm::mat4 proj(float aspect) const;

    // Helpers
    glm::vec3 front() const;
    glm::vec3 right() const;
    glm::vec3 up() const;
};
