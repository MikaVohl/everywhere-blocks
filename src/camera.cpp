#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>

static inline float rad(float deg) { return glm::radians(deg); }

glm::vec3 Camera::front() const {
    glm::vec3 f;
    f.x = cos(rad(yaw)) * cos(rad(pitch));
    f.y = sin(rad(pitch));
    f.z = sin(rad(yaw)) * cos(rad(pitch));
    return glm::normalize(f);
}
glm::vec3 Camera::right() const {
    return glm::normalize(glm::cross(front(), glm::vec3(0,1,0)));
}
glm::vec3 Camera::up() const {
    return glm::normalize(glm::cross(right(), front()));
}

glm::mat4 Camera::view() const {
    return glm::lookAt(pos, pos + front(), glm::vec3(0,1,0));
}

glm::mat4 Camera::proj(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearp, farp);
}
