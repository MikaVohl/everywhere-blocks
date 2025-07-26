cmake_minimum_required(VERSION 3.20)
project(TinyCraft CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(glfw3 REQUIRED)
find_package(glm REQUIRED) # we’ll use it soon; harmless if unused right now

add_executable(tinycraft
    src/main.cpp
    src/camera.cpp
    src/gfx/Shader.cpp
    src/gfx/Texture.cpp
    src/gfx/Mesh.cpp
    external/stb_image.cpp
)

# Stop GLFW from including legacy GL headers & silence Apple’s deprecation warning
target_compile_definitions(tinycraft PRIVATE GLFW_INCLUDE_NONE GL_SILENCE_DEPRECATION)

# Link frameworks explicitly (GLFW usually does this, but let's be explicit)
target_link_libraries(tinycraft
    PRIVATE
        glfw
        glm::glm
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
)
