#include <glm/vec3.hpp>

enum class BlockId {
    Tile,
    Turf
};

struct Block {
    glm::ivec3 pos;
    BlockId id;
};