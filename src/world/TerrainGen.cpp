#include <vector>
#include <glm/vec3.hpp>
#include "Block.hpp"
#include "TerrainGen.hpp"

std::vector<Block> makeTerrain(int terrainWidth, int terrainHeight) {
    std::vector<Block> blocks;
    for (int x = -terrainWidth / 2; x < terrainWidth / 2; ++x) {
        for (int z = -terrainWidth / 2; z < terrainWidth / 2; ++z) {
            for (int y = 0; y < terrainHeight; ++y) {
                int blockId = (y >= terrainHeight - 2) ? 1 : 0; // turf on top layer, tile below
                if (y < terrainHeight - 1 || (rand() % 10) < 4) {
                    blocks.push_back(Block{glm::vec3(float(x), float(y), float(z)), static_cast<BlockId>(blockId)});
                }
            }
        }
    }
    return blocks;
}