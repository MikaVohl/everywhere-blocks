#pragma once
#include <array>
#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>

enum class Key : int {
    W = GLFW_KEY_W, A = GLFW_KEY_A, S = GLFW_KEY_S, D = GLFW_KEY_D, Space = GLFW_KEY_SPACE, Shift = GLFW_KEY_LEFT_SHIFT, Escape = GLFW_KEY_ESCAPE,
    P = GLFW_KEY_P, O = GLFW_KEY_O, Left = GLFW_KEY_LEFT, Right = GLFW_KEY_RIGHT, Up = GLFW_KEY_UP, Down = GLFW_KEY_DOWN, N1 = GLFW_KEY_1,
    N2 = GLFW_KEY_2, N3 = GLFW_KEY_3, N4 = GLFW_KEY_4, N5 = GLFW_KEY_5, N6 = GLFW_KEY_6, N7 = GLFW_KEY_7, N8 = GLFW_KEY_8, N9 = GLFW_KEY_9,
    N0 = GLFW_KEY_0
};

enum class Mouse : int {
    Left  = GLFW_MOUSE_BUTTON_LEFT, Right = GLFW_MOUSE_BUTTON_RIGHT
};

class Input {
public:
    explicit Input(GLFWwindow* win) { init(win); } // explicit constructor to avoid implicit conversions

    void update();
    bool isDown(Key k) const;
    bool wasPressed(Key k) const;
    bool wasReleased(Key k) const;
    bool isDown(Mouse m) const;
    bool wasPressed(Mouse m) const;
    bool wasReleased(Mouse m) const;

    glm::vec2 mousePos() const { return mousePos_; }
    glm::vec2 mouseDelta() const { return mouseDelta_; }

    double scrollDelta(); // resets automatically after read

private:
    void init(GLFWwindow* win);

    GLFWwindow* win_ = nullptr;

    std::array<uint8_t, GLFW_KEY_LAST + 1> keyCurr_{};
    std::array<uint8_t, GLFW_KEY_LAST + 1> keyPrev_{};
    std::array<uint8_t, GLFW_MOUSE_BUTTON_LAST + 1> mouseCurr_{};
    std::array<uint8_t, GLFW_MOUSE_BUTTON_LAST + 1> mousePrev_{};

    glm::vec2 mousePos_{}, mouseDelta_{};
    double scroll_ = 0.0;
};
