#include "Shader.hpp"
#include <string>
#include <stdexcept>
#include <OpenGL/gl3.h>


ShaderProgram::ShaderProgram(const char* vertSrc, const char* fragSrc) {
    GLuint vs = compile(GL_VERTEX_SHADER, vertSrc);
    GLuint fs = compile(GL_FRAGMENT_SHADER, fragSrc);
    program_ = link(vs, fs); // a program represents the complete set of GPU instructions for rendering
    glDeleteShader(vs);
    glDeleteShader(fs);
}

ShaderProgram::~ShaderProgram() { // destructor to clean up resources
    if (program_) glDeleteProgram(program_);
}

ShaderProgram::ShaderProgram(ShaderProgram&& other) noexcept : program_(other.program_) { // move constructor
    other.program_ = 0; // transfer ownership of the program resource
}

ShaderProgram& ShaderProgram::operator=(ShaderProgram&& other) noexcept { // move assignment operator
    if (this != &other) {
        if (program_) glDeleteProgram(program_);
        program_ = other.program_;
        other.program_ = 0;
    }
    return *this;
}

void ShaderProgram::use() const { // wrapper to set the current shader program
    glUseProgram(program_);
}

GLuint ShaderProgram::id() const { // wraper to get the OpenGL program ID
    return program_;
}


GLuint ShaderProgram::compile(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);
    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0; glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
        std::string logStr(len, '\0');
        glGetShaderInfoLog(id, len, nullptr, logStr.data());
        throw std::runtime_error("Shader compile error: " + logStr);
    }
    return id;
}

GLuint ShaderProgram::link(GLuint vs, GLuint fs) {
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
        throw std::runtime_error("Program link error: " + logStr);
    }
    return prog;
}