#include <vector>
#include <glm/glm.hpp>
#include "../gfx/InstanceBuffer.hpp"

std::vector<BlockInstance> makeTerrain(int terrainWidth, int terrainHeight) {
    std::vector<BlockInstance> terrainBlocks;
    for (int x = -terrainWidth / 2; x < terrainWidth / 2; ++x) {
        for (int z = -terrainWidth / 2; z < terrainWidth / 2; ++z) {
            for (int y = 0; y < terrainHeight; ++y) {
                int texIdx = (y >= terrainHeight - 2) ? 1 : 0; // turf on top layer, tile below
                if (y < terrainHeight - 1 || (rand() % 10) < 4) {
                    terrainBlocks.push_back({glm::vec3(float(x), float(y), float(z)), texIdx});
                }
            }
        }
    }
    return terrainBlocks;
}