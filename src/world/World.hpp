#pragma once
#include <vector>
#include "Block.hpp"

class World {
public:
    World(std::vector<Block> blocks);
    std::vector<Block> blocks();
    BlockHitInfo raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const;
    void add(const Block& block);
    void remove(const glm::ivec3& pos);

private:
    std::vector<Block> blocks_;
};