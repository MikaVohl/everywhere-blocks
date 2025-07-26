#include <cstdio>
#include <cstdlib>
#include <string>
#include <cmath>

#define GLFW_INCLUDE_NONE
#include <OpenGL/gl3.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>


#include "camera.h"

struct CubeMesh {
    GLuint vao; // vao stands for "Vertex Array Object". It stores references to vertex attributes and buffers.
    GLuint vbo; // vbo stands for "Vertex Buffer Object". It stores vertex data like positions, normals, texture coordinates, etc. Array of positions (8 sets of xyz, 8 sets of normals, ...))
    GLuint ebo; // ebo stands for "Element Buffer Object". It stores indices that define how vertices are connected to form triangles.
    GLsizei indexCount;
};

CubeMesh createCubeMesh() {
    float verts[] = { // relative to the center of the cube (0,0,0)
        // front face
        -0.5f,-0.5f, 0.5f,
         0.5f,-0.5f, 0.5f,
         0.5f, 0.5f, 0.5f,
        -0.5f, 0.5f, 0.5f,
        // back face
        -0.5f,-0.5f,-0.5f,
         0.5f,-0.5f,-0.5f,
         0.5f, 0.5f,-0.5f,
        -0.5f, 0.5f,-0.5f,
    };
    unsigned idx[] = { // triangle indices
        // front
        0,1,2,  2,3,0,
        // right
        1,5,6,  6,2,1,
        // back
        5,4,7,  7,6,5,
        // left
        4,0,3,  3,7,4,
        // top
        3,2,6,  6,7,3,
        // bottom
        4,5,1,  1,0,4
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

static const char* kVS = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 instanceOffset;
uniform mat4 uVP;
void main() {
    vec3 pos = aPos + instanceOffset;
    gl_Position = uVP * vec4(pos, 1.0);
}
)";

static const char* kFS = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.5, 0.5, 0.5, 1.0);
}
)";

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

static GLuint compile(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::string logStr(len, '\0');
        glGetShaderInfoLog(id, len, nullptr, logStr.data());
        fprintf(stderr, "Shader compile error: %s\n", logStr.c_str());
        exit(1);
    }
    return id;
}
static GLuint link(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string logStr(len, '\0');
        glGetProgramInfoLog(prog, len, nullptr, logStr.data());
        fprintf(stderr, "Program link error: %s\n", logStr.c_str());
        exit(1);
    }
    return prog;
}

// ---------------------------------------------
// Input helpers
// ---------------------------------------------
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

    glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    GLuint vs = compile(GL_VERTEX_SHADER, kVS);
    GLuint fs = compile(GL_FRAGMENT_SHADER, kFS);
    GLuint prog = link(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint uVP = glGetUniformLocation(prog, "uVP");


    Camera cam;
    glfwSetWindowUserPointer(win, &cam);

    CubeMesh cube = createCubeMesh();
    std::vector<glm::vec3> instanceOffsets = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(2.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, -2.0f, 0.0f),
        glm::vec3(1.0f, -2.0f, 0.0f),
        glm::vec3(2.0f, -2.0f, 0.0f),
    };
    bool needUpload = true;
    const glm::vec3 kSpawn = glm::vec3(1.0f, -1.0f, 0.0f);

    GLuint instanceVBO;
    glGenBuffers(1, &instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceOffsets.size() * sizeof(glm::vec3), instanceOffsets.data(), GL_STATIC_DRAW);

    glBindVertexArray(cube.vao);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1); // Advance per instance
    glBindVertexArray(0);

    double lastTime = glfwGetTime();
    double lastX = 0.0, lastY = 0.0;
    bool firstMouse = true;
    // bool first = true;

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

            cam.yaw   += float(dx) * cam.mouseSensitivity;
            cam.pitch += float(dy) * cam.mouseSensitivity;
            cam.pitch = glm::clamp(cam.pitch, -89.0f, 89.0f);
        }

        processKeyboard(win, cam, dt);
        
        static bool prevP = false, prevO = false;
        bool nowP = glfwGetKey(win, GLFW_KEY_P) == GLFW_PRESS;
        bool nowO = glfwGetKey(win, GLFW_KEY_O) == GLFW_PRESS;

        if (nowP && !prevP) {
            if (std::find(instanceOffsets.begin(), instanceOffsets.end(), kSpawn) == instanceOffsets.end()) {
                instanceOffsets.push_back(kSpawn);
                needUpload = true;
            }
        }
        if (nowO && !prevO) {
            auto it = std::find(instanceOffsets.begin(), instanceOffsets.end(), kSpawn);
            if (it != instanceOffsets.end()) {
                instanceOffsets.erase(it);
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
        glViewport(0,0,w,h);
        glClearColor(0.1f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (needUpload) {
            glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
            glBufferData(GL_ARRAY_BUFFER, instanceOffsets.size() * sizeof(glm::vec3), instanceOffsets.data(), GL_STATIC_DRAW);
            needUpload = false;
        }

        glUseProgram(prog);
        glUniformMatrix4fv(uVP, 1, GL_FALSE, glm::value_ptr(vp));
        glBindVertexArray(cube.vao);
        glDrawElementsInstanced(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0, instanceOffsets.size());
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