#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


#include "camera.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture.hpp"
#include "gfx/Mesh.hpp"
#include "gfx/InstanceBuffer.hpp"
#include "gfx/Renderer.hpp"
#include "world/Block.hpp"
#include "world/TerrainGen.hpp"
#include "world/World.hpp"
#include "input/Input.hpp"

#define GL_CALL(x) do { \
    x; \
    GLenum err = glGetError(); \
    if (err != GL_NO_ERROR) { \
        fprintf(stderr, "GL error 0x%x at %s:%d\n", err, __FILE__, __LINE__); \
    } \
} while(0)


// GLSL (OpenGL Shading Language) shaders for vertex and fragment
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

const double PLACE_COOLDOWN = 0.1; // seconds
const double BREAK_COOLDOWN = 0.1; // seconds
const float PLAYER_REACH = 10.0f;
const int TERRAIN_WIDTH = 32;
const int TERRAIN_HEIGHT = 4;

static void glfw_error_callback(int code, const char* desc) {
    fprintf(stderr, "GLFW error %d: %s\n", code, desc);
}
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}
static void scroll_callback(GLFWwindow* win, double xoffset, double yoffset) {
    // change camera FOV
    Camera* cam = static_cast<Camera*>(glfwGetWindowUserPointer(win));
    cam->fov -= float(yoffset);
}

static void processKeyboard(Input& input, GLFWwindow* win, Camera& cam, float dt) {
    glm::vec3 f = cam.front();
    glm::vec3 r = cam.right();
    glm::vec3 u = glm::vec3(0,1,0); // world up
    float v = cam.moveSpeed * dt;

    if (input.isDown(Key::W)) cam.pos += f * v;
    if (input.isDown(Key::S)) cam.pos -= f * v;
    if (input.isDown(Key::D)) cam.pos += r * v;
    if (input.isDown(Key::A)) cam.pos -= r * v;
    if (input.isDown(Key::Space)) cam.pos += u * v;
    if (input.isDown(Key::Shift)) cam.pos -= u * v;
}

int main() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(1280, 720, "TinyCraft", nullptr, nullptr);
    Input input(win);
    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);

    Texture2D tex[2]; // Define two textures
    tex[0].load("assets/tile.png");
    tex[1].load("assets/turf.png");

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    CubeMesh cube;
    Renderer renderer(kVS, kFS, cube);
    glUseProgram(renderer.shader().id());
    glUniform1iv(glGetUniformLocation(renderer.shader().id(), "uTex"), 2, (int[]){0,1});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[0].texID);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex[1].texID);

    Camera cam;
    glfwSetWindowUserPointer(win, &cam);

    World world(makeTerrain(TERRAIN_WIDTH, TERRAIN_HEIGHT));

    bool needUpload = true;
    const glm::ivec3 kSpawn = glm::vec3(1.0f, -1.0f, 0.0f);

    InstanceVBO instanceVBO;
    renderer.buildInstanceBuffer(world.blocks(), instanceVBO);

    glBindVertexArray(cube.getVAO());
    glBindBuffer(GL_ARRAY_BUFFER, cube.getVBO()); // Bind cube.vbo, set per-vertex attributes 0 & 2
    glEnableVertexAttribArray(0); // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribDivisor(0, 0); // per-vertex
    glEnableVertexAttribArray(2); // UV attribute (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribDivisor(2, 0); // per-vertex

    instanceVBO.bind(); // Bind instanceVBO, set per-instance attributes 1 & 3
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(BlockInstance), (void*)0);
    glVertexAttribDivisor(1, 1); // per-instance
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 1, GL_INT, sizeof(BlockInstance), (void*)offsetof(BlockInstance, texIndex));
    glVertexAttribDivisor(3, 1); // per-instance

    glBindVertexArray(0); // Unbind VAO

    double lastTime = glfwGetTime();
    double lastX = 0.0, lastY = 0.0;
    bool firstMouse = true;
    double lastPlaceTime = 0.0;
    double lastBreakTime = 0.0;

    while (!glfwWindowShouldClose(win)) {
        double now = glfwGetTime();
        float dt = float(now - lastTime);
        lastTime = now;

        glfwPollEvents();
        input.update();

        if (input.isDown(Key::Escape)) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        // on mouse click enter cursor lock
        if (input.isDown(Mouse::Left)) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true; // reset mouse position
        }

        // Mouse look
        if (glfwGetInputMode(win, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            glm::vec2 mousePos = input.mousePos();
            if (firstMouse) {
                lastX = mousePos.x; lastY = mousePos.y; firstMouse = false;
            }
            double dx = mousePos.x - lastX;
            double dy = lastY - mousePos.y; // invert y
            lastX = mousePos.x; lastY = mousePos.y;

            cam.yaw += float(dx) * cam.mouseSensitivity;
            cam.pitch += float(dy) * cam.mouseSensitivity;
            cam.pitch = glm::clamp(cam.pitch, -89.0f, 89.0f);
        }

        // on mouse left click, check for block under crosshair and remove it
        static bool prevLeft = false, prevRight = false;
        bool nowLeft = input.isDown(Mouse::Left);
        bool nowRight = input.isDown(Mouse::Right);

        if (nowLeft && !prevLeft) {
            BlockHitInfo hit = world.raycast(cam.pos, cam.front(), PLAYER_REACH);
            if (now - lastBreakTime > BREAK_COOLDOWN && hit.blockIndex != -1) {
                world.remove(world.blocks()[hit.blockIndex].pos);
                needUpload = true;
                lastBreakTime = now; // reset break cooldown
            }
        }
        if (nowRight && !prevRight) {
            BlockHitInfo hit = world.raycast(cam.pos, cam.front(), PLAYER_REACH);
            if (hit.blockIndex != -1 && hit.faceIndex != -1) {
                glm::ivec3 faceNormals[] = {
                    glm::ivec3(0, -1, 0), // bottom
                    glm::ivec3(1, 0, 0), // right
                    glm::ivec3(0, 1, 0), // top
                    glm::ivec3(-1, 0, 0), // left
                    glm::ivec3(0, 0, 1), // front
                    glm::ivec3(0, 0, -1) // back
                };
                glm::ivec3 spawnPos = hit.blockPos + faceNormals[hit.faceIndex];
                bool exists = false;
                for (const auto& b : world.blocks()) {
                    if (b.pos == spawnPos) { exists = true; break; }
                }
                if (now - lastPlaceTime > PLACE_COOLDOWN && !exists) {
                    world.add(Block{spawnPos, BlockId::Tile});
                    needUpload = true;
                    lastPlaceTime = now; // reset place cooldown
                }
            }
        }
        prevLeft = nowLeft;
        prevRight = nowRight;

        processKeyboard(input, win, cam, dt);

        int w, h; glfwGetFramebufferSize(win, &w, &h);
        float aspect = (h>0) ? (float)w / (float)h : 1.0f;

        glm::mat4 v = cam.view();
        glm::mat4 p = cam.proj(aspect);
        glm::mat4 vp = p * v;

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);  

        glViewport(0,0,w,h);
        glClearColor(0.1f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (needUpload) {
            renderer.buildInstanceBuffer(world.blocks(), instanceVBO);
            needUpload = false;
        }

        renderer.draw(vp, world.blocks().size());

        glfwSwapBuffers(win);
    }


    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}