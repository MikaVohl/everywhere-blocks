#include "Application.hpp"
#include "../gfx/Mesh.hpp"
#include "../gfx/Texture.hpp"
#include "../world/TerrainGen.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <array>

static const char* kGuiVS = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

static const char* kGuiFS = R"(
#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
    FragColor = texture(uTex, vUV);
}
)";

Application::Application(int width, int height, const char* title) {
    if (!glfwInit()) throw std::runtime_error("GLFW init failed"); // Initialize GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // OpenGL version 3._
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // OpenGL version _.3 -> makes 3.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Use core profile
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // required for macOS
#endif
    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr); // creates a window
    glfwMakeContextCurrent(window_); // specify the above window as the current context
    glfwSwapInterval(1); // the number of screen updates to wait from the time glfwSwapBuffers was called before swapping the buffers and returning. Sets framerate to monitor refresh rate
    input_ = std::make_unique<Input>(window_);
    tex_[0].load("assets/tile.png");
    tex_[1].load("assets/turf.png");
    tex_[2].load("assets/cardboard.png");
    crosshairTex_.load("assets/crosshair.png");
    glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Hide and disable mouse cursor when in the window
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
    uniform sampler2D uTex[3];
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
    glUniform1iv(glGetUniformLocation(renderer_->shader().id(), "uTex"), 3, (int[]){0,1,2});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_[0].texID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex_[1].texID);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tex_[2].texID);
    camera_ = std::make_unique<Camera>();
    glfwSetWindowUserPointer(window_, camera_.get());
    world_ = std::make_unique<World>(makeTerrain(32, 4));
    renderer_->buildInstanceBuffer(world_->blocks(), instanceVBO_);
    needUpload_ = true;
    initHUD();
    lastTime_ = glfwGetTime();
}

Application::~Application() {
    if (hudEbo_) glDeleteBuffers(1, &hudEbo_);
    if (hudVbo_) glDeleteBuffers(1, &hudVbo_);
    if (hudVao_) glDeleteVertexArrays(1, &hudVao_);
    glfwDestroyWindow(window_);
    glfwTerminate();
}

void Application::initHUD() {
    // Create GUI shader
    guiShader_ = std::make_unique<ShaderProgram>(kGuiVS, kGuiFS);
    guiShader_->use();
    glUniform1i(glGetUniformLocation(guiShader_->id(), "uTex"), 10); // crosshair on unit 10

    glGenVertexArrays(1, &hudVao_);
    glBindVertexArray(hudVao_);

    glGenBuffers(1, &hudVbo_);
    glBindBuffer(GL_ARRAY_BUFFER, hudVbo_);
    glBufferData(GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW); // 4 verts, (x,y,u,v)

    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1); // aUV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    static const unsigned idx[6] = { 0,2,1,   2,0,3 };
    glGenBuffers(1, &hudEbo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, hudEbo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glBindVertexArray(0);
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
        drawHUD(w, h);
        glfwSwapBuffers(window_);
    }
}

void Application::drawHUD(int fbw, int fbh) {
    if (fbw <= 0 || fbh <= 0) return;

    // Convert desired pixel size to NDC half-extent:
    float hx = (float)crosshairPx_ / (float)fbw;
    float hy = (float)crosshairPx_ / (float)fbh;

    const float verts[4 * 4] = {
        // x,    y,    u, v
        -hx,  +hy,   0, 1,
        +hx,  +hy,   1, 1,
        +hx,  -hy,   1, 0,
        -hx,  -hy,   0, 0,
    };

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    guiShader_->use();
    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_2D, crosshairTex_.texID);

    glBindVertexArray(hudVao_);
    glBindBuffer(GL_ARRAY_BUFFER, hudVbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void Application::processInput(float dt) {
    glm::vec3 f = camera_->front();
    f.y = 0.0f; // ignore vertical component for movement
    f = glm::normalize(f); // normalize to ensure consistent speed
    glm::vec3 r = camera_->right();
    r.y = 0.0f; // ignore vertical component for movement
    r = glm::normalize(r); // normalize to ensure consistent speed
    glm::vec3 u = glm::vec3(0,1,0);
    float v = camera_->moveSpeed * dt;
    if (input_->isDown(Key::W)) camera_->pos += f * v;
    if (input_->isDown(Key::S)) camera_->pos -= f * v;
    if (input_->isDown(Key::D)) camera_->pos += r * v;
    if (input_->isDown(Key::A)) camera_->pos -= r * v;
    if (input_->isDown(Key::Space)) camera_->pos += u * v;
    if (input_->isDown(Key::Shift)) camera_->pos -= u * v;
    if (input_->isDown(Key::Escape)) glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (input_->isDown(Mouse::Left)) {
        glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse_ = true;
    }
    if (input_->wasPressed(Key::N1)) heldBlockId_ = 0; // Tile
    if (input_->wasPressed(Key::N2)) heldBlockId_ = 1; // Turf
    if (input_->wasPressed(Key::N3)) heldBlockId_ = 2; // Cardboard
    if (input_->wasPressed(Key::N0)) heldBlockId_ = -1; // No block held
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
        camera_->pitch = glm::clamp(camera_->pitch, -89.9f, 89.9f);
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
                world_->add(Block{spawnPos, static_cast<BlockId>(heldBlockId_)});
                needUpload_ = true;
                lastPlaceTime_ = now;
            }
        }
    }
    prevLeft = nowLeft;
    prevRight = nowRight;
}
