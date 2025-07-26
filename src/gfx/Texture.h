#pragma once
#include "../../external/stb_image.h"
#include <OpenGL/gl3.h>
#include <string>

class Texture2D {
public:
    GLuint texID;
    int width, height, channels;

    bool load(const std::string& path);
};