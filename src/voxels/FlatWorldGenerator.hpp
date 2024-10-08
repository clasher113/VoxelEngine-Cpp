#pragma once

#include "typedefs.hpp"
#include "WorldGenerator.hpp"

struct voxel;
class Content;

class FlatWorldGenerator : WorldGenerator {
public:
    FlatWorldGenerator(const Content* content) : WorldGenerator(content) {
    }

    void generate(voxel* voxels, int x, int z, int seed);
};
