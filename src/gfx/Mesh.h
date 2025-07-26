#include <OpenGL/gl3.h>

class CubeMesh {
public:
    CubeMesh();
    ~CubeMesh();

    GLuint getVAO() const { return vao; }
    GLuint getVBO() const { return vbo; }
    GLsizei getIndexCount() const { return indexCount; }

private:
    GLuint vao = 0; // Vertex Array Object
    GLuint vbo = 0; // Vertex Buffer Object
    GLuint ebo = 0; // Element Buffer Object
    GLsizei indexCount = 0; // Number of indices in the element buffer
};