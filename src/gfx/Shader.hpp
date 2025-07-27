#pragma once // Header guard to prevent multiple inclusions
#include <OpenGL/gl3.h>

class ShaderProgram {
public:
    ShaderProgram(const char* vertSrc, const char* fragSrc);
    ~ShaderProgram();

    ShaderProgram(const ShaderProgram&) = delete; // disable copy constructor. otherwise c++ compiler will generate a default copy constructor
    ShaderProgram& operator=(const ShaderProgram&) = delete; // disable copy assignment operator
    ShaderProgram(ShaderProgram&& other) noexcept;
    ShaderProgram& operator=(ShaderProgram&& other) noexcept;

    void use() const;
    GLuint id() const;

private:
    GLuint program_ = 0; // OpenGL program ID
    static GLuint compile(GLenum type, const char* src);
    static GLuint link(GLuint vs, GLuint fs);
};