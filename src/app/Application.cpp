#include "Application.hpp"
#include "../gfx/Mesh.hpp"
#include "../gfx/Texture.hpp"
#include "../world/TerrainGen.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <array>

#define GL_CALL(x) do { \
    x; \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        fprintf(stderr, "GL error 0x%x at %s:%d\n", err, __FILE__, __LINE__); \
    } \
} while(0)

static void glfw_error_callback(int code, const char* desc) {
    fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

Application::Application(int width, int height, const char* title) {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) throw std::runtime_error("GLFW init failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) { glfwTerminate(); throw std::runtime_error("Window creation failed"); }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
    input_ = std::make_unique<Input>(window_);
    tex_[0].load("assets/tile.png");
    tex_[1].load("assets/turf.png");
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    static const char* kVS = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 instanceOffset;
    layout (location = 2) in vec2 aUV;
    layout (location = 3) in int texIndex;
    out vec2 vUV;
    flat out int vTexIndex;

    uniform mat4 uVP;
    void main() {
        vec3 pos = aPos + instanceOffset;
        vUV = aUV;
        vTexIndex = texIndex;
        gl_Position = uVP * vec4(pos, 1.0);
    }
    )";

    static const char* kFS = R"(
    #version 330 core
    in vec2 vUV;
    flat in int vTexIndex;
    uniform sampler2D uTex[2];
    out vec4 FragColor;
    void main() {
        FragColor = texture(uTex[vTexIndex], vUV);
    }
    )";

    cube_ = std::make_unique<CubeMesh>();
    instanceVBO_.init();
    renderer_ = std::make_unique<Renderer>(kVS, kFS, *cube_);
    renderer_->setupAttributes(*cube_, instanceVBO_);
    
    glUseProgram(renderer_->shader().id());
    glUniform1iv(glGetUniformLocation(renderer_->shader().id(), "uTex"), 2, (int[]){0,1});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_[0].texID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_[1].texID);
    camera_ = std::make_unique<Camera>();
    glfwSetWindowUserPointer(window_, camera_.get());
    world_ = std::make_unique<World>(makeTerrain(32, 4));
    renderer_->buildInstanceBuffer(world_->blocks(), instanceVBO_);
    needUpload_ = true;
    lastTime_ = glfwGetTime();
}

Application::~Application() {
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void Application::run() {
    while (!glfwWindowShouldClose(window_)) {
        double now = glfwGetTime();
        float dt = float(now - lastTime_);
        lastTime_ = now;
        glfwPollEvents();
        input_->update();
        processInput(dt);
        handleMouseLook();
        handleBlockActions();
        int w, h; glfwGetFramebufferSize(window_, &w, &h);
        float aspect = (h>0) ? (float)w / (float)h : 1.0f;
        glm::mat4 v = camera_->view();
        glm::mat4 p = camera_->proj(aspect);
        glm::mat4 vp = p * v;
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glViewport(0,0,w,h);
        glClearColor(0.1f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (needUpload_) {
            renderer_->buildInstanceBuffer(world_->blocks(), instanceVBO_);
            needUpload_ = false;
        }
        renderer_->draw(vp, world_->blocks().size());
        glfwSwapBuffers(window_);
    }
}

void Application::processInput(float dt) {
    glm::vec3 f = camera_->front();
    glm::vec3 r = camera_->right();
    glm::vec3 u = glm::vec3(0,1,0);
    float v = camera_->moveSpeed * dt;
    if (input_->isDown(Key::W)) camera_->pos += f * v;
    if (input_->isDown(Key::S)) camera_->pos -= f * v;
    if (input_->isDown(Key::D)) camera_->pos += r * v;
    if (input_->isDown(Key::A)) camera_->pos -= r * v;
    if (input_->isDown(Key::Space)) camera_->pos += u * v;
    if (input_->isDown(Key::Shift)) camera_->pos -= u * v;
    if (input_->isDown(Key::Escape)) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (input_->isDown(Mouse::Left)) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse_ = true;
    }
}

void Application::handleMouseLook() {
    if (glfwGetInputMode(window_, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
        glm::vec2 mousePos = input_->mousePos();
        if (firstMouse_) {
            lastX_ = mousePos.x; lastY_ = mousePos.y; firstMouse_ = false;
        }
        double dx = mousePos.x - lastX_;
        double dy = lastY_ - mousePos.y;
        lastX_ = mousePos.x; lastY_ = mousePos.y;
        camera_->yaw += float(dx) * camera_->mouseSensitivity;
        camera_->pitch += float(dy) * camera_->mouseSensitivity;
        camera_->pitch = glm::clamp(camera_->pitch, -89.0f, 89.0f);
    }
}

void Application::handleBlockActions() {
    static bool prevLeft = false, prevRight = false;
    bool nowLeft = input_->isDown(Mouse::Left);
    bool nowRight = input_->isDown(Mouse::Right);
    double now = glfwGetTime();
    constexpr double PLACE_COOLDOWN = 0.1;
    constexpr double BREAK_COOLDOWN = 0.1;
    constexpr float PLAYER_REACH = 10.0f;
    if (nowLeft && !prevLeft) {
        BlockHitInfo hit = world_->raycast(camera_->pos, camera_->front(), PLAYER_REACH);
        if (now - lastBreakTime_ > BREAK_COOLDOWN && hit.blockIndex != -1) {
            world_->remove(world_->blocks()[hit.blockIndex].pos);
            needUpload_ = true;
            lastBreakTime_ = now;
        }
    }
    if (nowRight && !prevRight) {
        BlockHitInfo hit = world_->raycast(camera_->pos, camera_->front(), PLAYER_REACH);
        if (hit.blockIndex != -1 && hit.faceIndex != -1) {
            glm::ivec3 faceNormals[] = {
                glm::ivec3(0, -1, 0),
                glm::ivec3(1, 0, 0),
                glm::ivec3(0, 1, 0),
                glm::ivec3(-1, 0, 0),
                glm::ivec3(0, 0, 1),
                glm::ivec3(0, 0, -1)
            };
            glm::ivec3 spawnPos = hit.blockPos + faceNormals[hit.faceIndex];
            bool exists = false;
            for (const auto& b : world_->blocks()) {
                if (b.pos == spawnPos) { exists = true; break; }
            }
            if (now - lastPlaceTime_ > PLACE_COOLDOWN && !exists) {
                world_->add(Block{spawnPos, BlockId::Tile});
                needUpload_ = true;
                lastPlaceTime_ = now;
            }
        }
    }
    prevLeft = nowLeft;
    prevRight = nowRight;
}
