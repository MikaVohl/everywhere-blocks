#include "Input.hpp"
#include <array>
#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>


void Input::update() {
    keyPrev_   = keyCurr_;
    mousePrev_ = mouseCurr_;

    for (int k = GLFW_KEY_SPACE; k <= GLFW_KEY_LAST; ++k)
        keyCurr_[k] = glfwGetKey(win_, k) == GLFW_PRESS;

    for (int b = 0; b <= GLFW_MOUSE_BUTTON_LAST; ++b)
        mouseCurr_[b] = glfwGetMouseButton(win_, b) == GLFW_PRESS;

    double x, y;
    glfwGetCursorPos(win_, &x, &y);
    glm::vec2 newPos(x, y);
    mouseDelta_ = newPos - mousePos_;
    mousePos_   = newPos;
}

bool Input::isDown(Key k) const {
    return keyCurr_[static_cast<int>(k)];
}
bool Input::wasPressed(Key k) const {
    return keyCurr_[static_cast<int>(k)] && !keyPrev_[static_cast<int>(k)];
}
bool Input::wasReleased(Key k) const {
    return !keyCurr_[static_cast<int>(k)] && keyPrev_[static_cast<int>(k)];
}
bool Input::isDown(Mouse m) const {
    return mouseCurr_[static_cast<int>(m)];
}
bool Input::wasPressed(Mouse m) const {
    return mouseCurr_[static_cast<int>(m)] && !mousePrev_[static_cast<int>(m)];
}
bool Input::wasReleased(Mouse m) const {
    return !mouseCurr_[static_cast<int>(m)] && mousePrev_[static_cast<int>(m)];
}
double Input::scrollDelta() {
    double delta = scroll_;
    scroll_ = 0.0; // reset after reading
    return delta;
}
void Input::init(GLFWwindow* win) {
    win_ = win;
    std::fill(keyCurr_.begin(), keyCurr_.end(), 0);
    std::fill(keyPrev_.begin(), keyPrev_.end(), 0);
    std::fill(mouseCurr_.begin(), mouseCurr_.end(), 0);
    std::fill(mousePrev_.begin(), mousePrev_.end(), 0);
    mousePos_ = glm::vec2(0.0f);
    mouseDelta_ = glm::vec2(0.0f);
    scroll_ = 0.0;
}