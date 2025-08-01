#pragma once
#include <glm/vec3.hpp>

enum class BlockId {
    Tile,
    Turf,
    Cardboard
};

struct Block {
    glm::ivec3 pos;
    BlockId id;
};

struct BlockHitInfo {
    int blockIndex; // Index in blocks vector
    glm::ivec3 blockPos; // Position of the block
    int faceIndex; // Face index (0=bottom, 1=right, 2=top, 3=left, 4=front, 5=back)
    glm::ivec3 hitPos; // World position of intersection
    float distance; // Distance from ray origin to hit
};