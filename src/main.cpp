#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"


#include "camera.h"
#include "gfx/Shader.h"

struct CubeMesh {
    GLuint vao; // vao stands for "Vertex Array Object". It stores references to vertex attributes and buffers.
    GLuint vbo; // vbo stands for "Vertex Buffer Object". It stores vertex data like positions, normals, texture coordinates, etc. Array of positions (8 sets of xyz, 8 sets of normals, ...))
    GLuint ebo; // ebo stands for "Element Buffer Object". It stores indices that define how vertices are connected to form triangles.
    GLsizei indexCount;
};

CubeMesh createCubeMesh() {
    float verts[] = {
        // Front face (z = +0.5)
        -0.5f, -0.5f,  0.5f, 0.0f, 0.0f, // 0
         0.5f, -0.5f,  0.5f, 1.0f, 0.0f, // 1
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, // 2
        -0.5f,  0.5f,  0.5f, 0.0f, 1.0f, // 3

        // Right face (x = +0.5)
         0.5f, -0.5f,  0.5f, 0.0f, 0.0f, // 4
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // 5
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f, // 6
         0.5f,  0.5f,  0.5f, 0.0f, 1.0f, // 7

        // Back face (z = -0.5)
         0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // 8
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // 9
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, // 10
         0.5f,  0.5f, -0.5f, 0.0f, 1.0f, // 11

        // Left face (x = -0.5)
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // 12
        -0.5f, -0.5f,  0.5f, 1.0f, 0.0f, // 13
        -0.5f,  0.5f,  0.5f, 1.0f, 1.0f, // 14
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, // 15

        // Top face (y = +0.5)
        -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, // 16
         0.5f,  0.5f,  0.5f, 1.0f, 0.0f, // 17
         0.5f,  0.5f, -0.5f, 1.0f, 1.0f, // 18
        -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, // 19

        // Bottom face (y = -0.5)
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, // 20
         0.5f, -0.5f, -0.5f, 1.0f, 0.0f, // 21
         0.5f, -0.5f,  0.5f, 1.0f, 1.0f, // 22
        -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, // 23
    };
    unsigned idx[] = {
        // Front face
        0, 1, 2,  2, 3, 0,
        // Right face
        4, 5, 6,  6, 7, 4,
        // Back face
        8, 9,10, 10,11, 8,
        // Left face
        12,13,14, 14,15,12,
        // Top face
        16,17,18, 18,19,16,
        // Bottom face
        20,21,22, 22,23,20
    };
    CubeMesh mesh{};
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    mesh.indexCount = sizeof(idx)/sizeof(idx[0]);
    glBindVertexArray(0);
    return mesh;
}

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
struct BlockHitInfo {
    int blockIndex; // Index in blocks vector
    glm::vec3 blockPos; // Position of the block
    int faceIndex; // Face index (0=bottom, 1=right, 2=top, 3=left, 4=front, 5=back)
    glm::vec3 hitPos; // World position of intersection
    float distance; // Distance from ray origin to hit
};

static BlockHitInfo target_block_face(const Camera& cam, const std::vector<glm::vec3>& blocks) {
    glm::vec3 rayOrigin = cam.pos;
    glm::vec3 rayDirection = cam.front();
    BlockHitInfo bestHit{ -1, glm::vec3(0), -1, glm::vec3(0), PLAYER_REACH + 1.0f };

    for (size_t i = 0; i < blocks.size(); ++i) { // iterate over every block and check distance
        const glm::vec3& block = blocks[i]; // current block
        glm::vec3 blockMin = block - glm::vec3(0.5f); // most negative corner of the block
        glm::vec3 blockMax = block + glm::vec3(0.5f); // most positive corner of the block

        float tMin = -INFINITY, tMax = INFINITY; // tMin: the minimum t value of the intersection, tMax: the maximum t value of the intersection
        // t value: the distance along the ray where it intersects the block's slabs (parameterized line)

        // X slab
        if (rayDirection.x != 0.0f) {
            float tx1 = (blockMin.x - rayOrigin.x) / rayDirection.x;
            float tx2 = (blockMax.x - rayOrigin.x) / rayDirection.x;
            tMin = std::max(tMin, std::min(tx1, tx2));
            tMax = std::min(tMax, std::max(tx1, tx2));
        } else if (rayOrigin.x < blockMin.x || rayOrigin.x > blockMax.x) {
            continue;
        }

        // Y slab
        if (rayDirection.y != 0.0f) {
            float ty1 = (blockMin.y - rayOrigin.y) / rayDirection.y;
            float ty2 = (blockMax.y - rayOrigin.y) / rayDirection.y;
            tMin = std::max(tMin, std::min(ty1, ty2));
            tMax = std::min(tMax, std::max(ty1, ty2));
        } else if (rayOrigin.y < blockMin.y || rayOrigin.y > blockMax.y) {
            continue;
        }

        // Z slab
        if (rayDirection.z != 0.0f) {
            float tz1 = (blockMin.z - rayOrigin.z) / rayDirection.z;
            float tz2 = (blockMax.z - rayOrigin.z) / rayDirection.z;
            tMin = std::max(tMin, std::min(tz1, tz2));
            tMax = std::min(tMax, std::max(tz1, tz2));
        } else if (rayOrigin.z < blockMin.z || rayOrigin.z > blockMax.z) {
            continue;
        }

        if (tMax < tMin || tMin < 0 || tMin > PLAYER_REACH) continue;

        glm::vec3 intersection = rayOrigin + tMin * rayDirection;
        glm::vec3 offset = intersection - block;
        int face = -1;
        if (std::abs(offset.x) > std::abs(offset.y) && std::abs(offset.x) > std::abs(offset.z)) {
            face = (offset.x > 0) ? 1 : 3; // right or left
        } else if (std::abs(offset.y) > std::abs(offset.x) && std::abs(offset.y) > std::abs(offset.z)) {
            face = (offset.y > 0) ? 2 : 0; // top or bottom
        } else {
            face = (offset.z > 0) ? 4 : 5; // front or back
        }

        if (tMin < bestHit.distance) {
            bestHit = { int(i), block, face, intersection, tMin };
        }
    }
    return bestHit;
}

static void processKeyboard(GLFWwindow* win, Camera& cam, float dt) {
    glm::vec3 f = cam.front();
    glm::vec3 r = cam.right();
    glm::vec3 u = glm::vec3(0,1,0); // use world up
    // glm::vec3 u = cam.up(); // use camera up
    float v = cam.moveSpeed * dt;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) cam.pos += f * v;
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) cam.pos -= f * v;
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) cam.pos += r * v;
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) cam.pos -= r * v;
    if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS) cam.pos += u * v;
    if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cam.pos -= u * v;
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
    if (!win) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    // glfwSetScrollCallback(win, scroll_callback);

    int w1,h1,n1,w2,h2,n2;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data1 = stbi_load("assets/tile.png", &w1, &h1, &n1, 4); // RGBA
    unsigned char* data2 = stbi_load("assets/turf.png", &w2, &h2, &n2, 4); // RGBA

    GLuint tex[2];
    glGenTextures(2, tex);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, w1,h1,0,GL_RGBA,GL_UNSIGNED_BYTE,data1);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(data1);

    glBindTexture(GL_TEXTURE_2D, tex[1]);
    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8, w2,h2,0,GL_RGBA,GL_UNSIGNED_BYTE,data2);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_image_free(data2);

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    ShaderProgram shader(kVS, kFS);
    GLuint prog = shader.id();
    GLint uVP = glGetUniformLocation(prog, "uVP");

    glUseProgram(prog);
    glUniform1iv(glGetUniformLocation(prog, "uTex"), 2, (int[]){0,1});
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tex[1]);

    Camera cam;
    glfwSetWindowUserPointer(win, &cam);

    CubeMesh cube = createCubeMesh();
    struct BlockInstance {
        glm::vec3 pos;
        int texIndex;
    };
    std::vector<BlockInstance> terrainBlocks;
    for (int x = -TERRAIN_WIDTH / 2; x < TERRAIN_WIDTH / 2; ++x) {
        for (int z = -TERRAIN_WIDTH / 2; z < TERRAIN_WIDTH / 2; ++z) {
            for (int y = 0; y < TERRAIN_HEIGHT; ++y) {
                int texIdx = (y >= TERRAIN_HEIGHT - 2) ? 1 : 0; // turf on top layer, tile below
                if (y < TERRAIN_HEIGHT - 1 || (rand() % 10) < 4) {
                    terrainBlocks.push_back({glm::vec3(float(x), float(y), float(z)), texIdx});
                }
            }
        }
    }

    bool needUpload = true;
    const glm::vec3 kSpawn = glm::vec3(1.0f, -1.0f, 0.0f);

    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, terrainBlocks.size() * sizeof(BlockInstance), terrainBlocks.data(), GL_STATIC_DRAW);

    glBindVertexArray(cube.vao);
    glBindBuffer(GL_ARRAY_BUFFER, cube.vbo); // Bind cube.vbo, set per-vertex attributes 0 & 2
    glEnableVertexAttribArray(0); // Position attribute (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribDivisor(0, 0); // per-vertex
    glEnableVertexAttribArray(2); // UV attribute (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribDivisor(2, 0); // per-vertex

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); // Bind instanceVBO, set per-instance attributes 1 & 3
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

        if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        // on mouse click enter cursor lock
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true; // reset mouse position
        }

        // Mouse look
        if (glfwGetInputMode(win, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            double x, y;
            glfwGetCursorPos(win, &x, &y);
            if (firstMouse) {
                lastX = x; lastY = y; firstMouse = false;
            }
            double dx = x - lastX;
            double dy = lastY - y; // invert y
            lastX = x; lastY = y;

            cam.yaw += float(dx) * cam.mouseSensitivity;
            cam.pitch += float(dy) * cam.mouseSensitivity;
            cam.pitch = glm::clamp(cam.pitch, -89.0f, 89.0f);
        }

        // on mouse left click, check for block under crosshair and remove it
        static bool prevLeft = false, prevRight = false;
        bool nowLeft = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        bool nowRight = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

        if (nowLeft && !prevLeft) {
            std::vector<glm::vec3> blockPositions;
            for (const auto& b : terrainBlocks) blockPositions.push_back(b.pos);
            BlockHitInfo hit = target_block_face(cam, blockPositions);
            if (now - lastBreakTime > BREAK_COOLDOWN && hit.blockIndex != -1) {
                terrainBlocks.erase(terrainBlocks.begin() + hit.blockIndex);
                needUpload = true;
                lastBreakTime = now; // reset break cooldown
            }
        }
        if (nowRight && !prevRight) {
            std::vector<glm::vec3> blockPositions;
            for (const auto& b : terrainBlocks) blockPositions.push_back(b.pos);
            BlockHitInfo hit = target_block_face(cam, blockPositions);
            if (hit.blockIndex != -1 && hit.faceIndex != -1) {
                // Calculate spawn position: offset by 1 unit along the hit face normal
                glm::vec3 faceNormals[] = {
                    glm::vec3(0, -1, 0), // bottom
                    glm::vec3(1, 0, 0), // right
                    glm::vec3(0, 1, 0), // top
                    glm::vec3(-1, 0, 0), // left
                    glm::vec3(0, 0, 1), // front
                    glm::vec3(0, 0, -1) // back
                };
                glm::vec3 spawnPos = hit.blockPos + faceNormals[hit.faceIndex];
                bool exists = false;
                for (const auto& b : terrainBlocks) {
                    if (b.pos == spawnPos) { exists = true; break; }
                }
                if (now - lastPlaceTime > PLACE_COOLDOWN && !exists) {
                    terrainBlocks.push_back({spawnPos, 0}); // tile.png for placed blocks
                    needUpload = true;
                    lastPlaceTime = now; // reset place cooldown
                }
            }
        }
        prevLeft = nowLeft;
        prevRight = nowRight;

        processKeyboard(win, cam, dt);
        
        static bool prevP = false, prevO = false;
        bool nowP = glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS;
        bool nowO = glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS;

        if (nowP && !prevP) {
            bool exists = false;
            for (const auto& b : terrainBlocks) {
                if (b.pos == kSpawn) { exists = true; break; }
            }
            if (!exists) {
                terrainBlocks.push_back({kSpawn, 0}); // tile.png for placed blocks
                needUpload = true;
            }
        }
        if (nowO && !prevO) {
            auto it = terrainBlocks.begin();
            for (; it != terrainBlocks.end(); ++it) {
                if (it->pos == kSpawn) break;
            }
            if (it != terrainBlocks.end()) {
                terrainBlocks.erase(it);
                needUpload = true;
            }
        }
        prevP = nowP;
        prevO = nowO;

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
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, terrainBlocks.size() * sizeof(BlockInstance), terrainBlocks.data(), GL_STATIC_DRAW);
            needUpload = false;
        }

        glUseProgram(prog);
        glUniformMatrix4fv(uVP, 1, GL_FALSE, glm::value_ptr(vp));
        glBindVertexArray(cube.vao);
        glDrawElementsInstanced(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0, terrainBlocks.size());
        glBindVertexArray(0);

        glfwSwapBuffers(win);
    }

    glDeleteBuffers(1, &cube.ebo);
    glDeleteBuffers(1, &cube.vbo);
    glDeleteBuffers(1, &instanceVBO);
    glDeleteVertexArrays(1, &cube.vao);
    glDeleteProgram(prog);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}