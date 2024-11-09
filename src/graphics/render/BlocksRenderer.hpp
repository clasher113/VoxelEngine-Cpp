#pragma once

#include <stdlib.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "voxels/voxel.hpp"
#include "typedefs.hpp"

#include "voxels/Block.hpp"
#include "voxels/Chunk.hpp"
#include "voxels/VoxelsVolume.hpp"
#include "graphics/core/MeshData.hpp"
#include "maths/util.hpp"

class Content;
class Mesh;
class Block;
class Chunk;
class Chunks;
class VoxelsVolume;
class ChunksStorage;
class ContentGfxCache;
struct EngineSettings;
struct UVRegion;

class BlocksRenderer {
    static const glm::vec3 SUN_VECTOR;
    static const uint VERTEX_SIZE;
    const Content* const content;
    std::unique_ptr<float[]> vertexBuffer;
    std::unique_ptr<int[]> indexBuffer;
    size_t vertexOffset;
    size_t indexOffset, indexSize;
    size_t capacity;
    int voxelBufferPadding = 2;
    bool overflow = false;
    bool cancelled = false;
    const Chunk* chunk = nullptr;
    std::unique_ptr<VoxelsVolume> voxelsBuffer;

    const Block* const* blockDefsCache;
    const ContentGfxCache* const cache;
    const EngineSettings* settings;
    
    util::PseudoRandom randomizer;

    void vertex(const glm::vec3& coord, float u, float v, const glm::vec4& light);
    void index(int a, int b, int c, int d, int e, int f);

    void vertexAO(
        const glm::vec3& coord, float u, float v, 
        const glm::vec4& brightness,
        const glm::vec3& axisX,
        const glm::vec3& axisY,
        const glm::vec3& axisZ
    );
    void face(
        const glm::vec3& coord, 
        float w, float h, float d,
        const glm::vec3& axisX,
        const glm::vec3& axisY,
        const glm::vec3& axisZ,
        const UVRegion& region,
        const glm::vec4(&lights)[4],
        const glm::vec4& tint
    );
    void face(
        const glm::vec3& coord,
        const glm::vec3& X,
        const glm::vec3& Y,
        const glm::vec3& Z,
        const UVRegion& region,
        glm::vec4 tint,
        bool lights
    );
    void faceAO(
        const glm::vec3& coord,
        const glm::vec3& axisX,
        const glm::vec3& axisY,
        const glm::vec3& axisZ,
        const UVRegion& region,
        bool lights
    );
    void blockCube(
        const glm::ivec3& coord,
        const UVRegion(&faces)[6], 
        const Block& block, 
        blockstate states, 
        bool lights,
        bool ao
    );
    void blockAABB(
        const glm::ivec3& coord,
        const UVRegion(&faces)[6], 
        const Block* block, 
        ubyte rotation,
        bool lights,
        bool ambientOcclusion
    );
    void blockXSprite(
        int x, int y, int z, 
        const glm::vec3& size, 
        const UVRegion& face1, 
        const UVRegion& face2, 
        float spread
    );
    void blockCustomModel(
        const glm::ivec3& icoord,
        const Block* block, 
        ubyte rotation,
        bool lights,
        bool ao
    );

    bool isOpenForLight(int x, int y, int z) const;


    // Does block allow to see other blocks sides (is it transparent)
    inline bool isOpen(const glm::ivec3& pos, ubyte group) const {
        auto id = voxelsBuffer->pickBlockId(
            chunk->x * CHUNK_W + pos.x, pos.y, chunk->z * CHUNK_D + pos.z
        );
        if (id == BLOCK_VOID) {
            return false;
        }
        const auto& block = *blockDefsCache[id];
        if ((block.drawGroup != group && block.lightPassing) || !block.rt.solid) {
            return true;
        }
        return !id;
    }

    glm::vec4 pickLight(int x, int y, int z) const;
    glm::vec4 pickLight(const glm::ivec3& coord) const;
    glm::vec4 pickSoftLight(const glm::ivec3& coord, const glm::ivec3& right, const glm::ivec3& up) const;
    glm::vec4 pickSoftLight(float x, float y, float z, const glm::ivec3& right, const glm::ivec3& up) const;
    void render(const voxel* voxels);
public:
    BlocksRenderer(size_t capacity, const Content* content, const ContentGfxCache* cache, const EngineSettings* settings);
    virtual ~BlocksRenderer();

    void build(const Chunk* chunk, const ChunksStorage* chunks);
    std::shared_ptr<Mesh> render(const Chunk* chunk, const ChunksStorage* chunks);
    MeshData createMesh();
    VoxelsVolume* getVoxelsBuffer() const;

    bool isCancelled() const {
        return cancelled;
    }
};
