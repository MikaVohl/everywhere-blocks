#pragma once
#include "../input/Input.hpp"
#include "../gfx/Renderer.hpp"
#include "../world/World.hpp"
#include "../camera.hpp"
#include "../gfx/Texture.hpp"
#include <GLFW/glfw3.h>
#include <memory>

class Application {
public:
    Application(int width, int height, const char* title);
    ~Application();
    void run();
private:
    void processInput(float dt);
    void handleMouseLook();
    void handleBlockActions();

    GLFWwindow* window_ = nullptr;
    std::unique_ptr<Input> input_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<World> world_;
    std::unique_ptr<Camera> camera_;
    InstanceVBO instanceVBO_;
    Texture2D tex_[2];
    bool needUpload_ = true;
    double lastPlaceTime_ = 0.0;
    double lastBreakTime_ = 0.0;
    double lastTime_ = 0.0;
    double lastX_ = 0.0, lastY_ = 0.0;
    bool firstMouse_ = true;
    std::unique_ptr<CubeMesh> cube_;
};
