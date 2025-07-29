#include <vector>
#include "Block.hpp"
#include "World.hpp"
#include <glm/vec3.hpp>

World::World(std::vector<Block> blocks) {
    blocks_ = std::move(blocks);
}

const std::vector<Block>& World::blocks() const {
    return blocks_;
}

BlockHitInfo World::raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance) const {
    BlockHitInfo bestHit{ -1, glm::vec3(0), -1, glm::vec3(0), maxDistance + 1.0f };

    for (size_t i = 0; i < blocks_.size(); ++i) { // iterate over every block and check distance
        const Block& block = blocks_[i]; // current block
        glm::vec3 blockMin = glm::vec3(block.pos) - glm::vec3(0.5f); // most negative corner of the block
        glm::vec3 blockMax = glm::vec3(block.pos) + glm::vec3(0.5f); // most positive corner of the block

        float tMin = -INFINITY, tMax = INFINITY; // tMin: the minimum t value of the intersection, tMax: the maximum t value of the intersection
        // t value: the distance along the ray where it intersects the block's slabs (parameterized line)

        // X slab
        if (direction.x != 0.0f) {
            float tx1 = (blockMin.x - origin.x) / direction.x;
            float tx2 = (blockMax.x - origin.x) / direction.x;
            tMin = std::max(tMin, std::min(tx1, tx2));
            tMax = std::min(tMax, std::max(tx1, tx2));
        } else if (origin.x < blockMin.x || origin.x > blockMax.x) {
            continue;
        }

        // Y slab
        if (direction.y != 0.0f) {
            float ty1 = (blockMin.y - origin.y) / direction.y;
            float ty2 = (blockMax.y - origin.y) / direction.y;
            tMin = std::max(tMin, std::min(ty1, ty2));
            tMax = std::min(tMax, std::max(ty1, ty2));
        } else if (origin.y < blockMin.y || origin.y > blockMax.y) {
            continue;
        }

        // Z slab
        if (direction.z != 0.0f) {
            float tz1 = (blockMin.z - origin.z) / direction.z;
            float tz2 = (blockMax.z - origin.z) / direction.z;
            tMin = std::max(tMin, std::min(tz1, tz2));
            tMax = std::min(tMax, std::max(tz1, tz2));
        } else if (origin.z < blockMin.z || origin.z > blockMax.z) {
            continue;
        }

        if (tMax < tMin || tMin < 0 || tMin > maxDistance) continue;

        glm::vec3 intersection = origin + tMin * direction;
        glm::vec3 offset = intersection - glm::vec3(block.pos);
        int face = -1;
        if (std::abs(offset.x) > std::abs(offset.y) && std::abs(offset.x) > std::abs(offset.z)) {
            face = (offset.x > 0) ? 1 : 3; // right or left
        } else if (std::abs(offset.y) > std::abs(offset.x) && std::abs(offset.y) > std::abs(offset.z)) {
            face = (offset.y > 0) ? 2 : 0; // top or bottom
        } else {
            face = (offset.z > 0) ? 4 : 5; // front or back
        }

        if (tMin < bestHit.distance) {
            bestHit = { int(i), glm::vec3(block.pos), face, intersection, tMin };
        }
    }
    return bestHit;
}

void World::add(const Block& block) {
    blocks_.push_back(block);
}

void World::remove(const glm::ivec3& pos) {
    blocks_.erase(std::remove_if(blocks_.begin(), blocks_.end(), [&pos](const Block& block) { return block.pos == pos; }), blocks_.end());
}