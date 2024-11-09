#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <GL/glew.h>
#include <string>

#include <glm/glm.hpp>

#include "typedefs.hpp"

class Level;
class Player;
class Camera;
class Batch3D;
class LineBatch;
class ChunksRenderer;
class ParticlesRenderer;
class Shader;
class Frustum;
class Engine;
class Chunks;
class LevelFrontend;
class Skybox;
class PostProcessing;
class DrawContext;
class ModelBatch;
class Assets;
class Emitter;
struct EngineSettings;

namespace model {
    struct Model;
}

class WorldRenderer {
    Engine* engine;
    Level* level;
    Player* player;
    std::unique_ptr<Frustum> frustumCulling;
    std::unique_ptr<LineBatch> lineBatch;
    std::unique_ptr<ChunksRenderer> renderer;
    std::unique_ptr<Skybox> skybox;
    std::unique_ptr<Batch3D> batch3d;
    std::unique_ptr<ModelBatch> modelBatch;
    
    float timer = 0.0f;

    bool drawChunk(size_t index, const Camera& camera, Shader* shader, bool culling);
    void drawChunks(Chunks* chunks, const  Camera& camera, Shader* shader);

    /// @brief Render block selection lines
    void renderBlockSelection();

    void renderHands(const Camera& camera, const Assets& assets, float delta);
    
    /// @brief Render lines (selection and debug)
    /// @param camera active camera
    /// @param linesShader shader used
    void renderLines(
        const Camera& camera, Shader* linesShader, const DrawContext& pctx
    );

    /// @brief Render all debug lines (chunks borders, coord system guides)
    /// @param context graphics context
    /// @param camera active camera
    /// @param linesShader shader used
    void renderDebugLines(
        const DrawContext& context, 
        const Camera& camera, 
        Shader* linesShader
    );

    void renderBlockOverlay(const DrawContext& context, const Assets& assets);

    void setupWorldShader(
        Shader* shader,
        const Camera& camera,
        const EngineSettings& settings,
        float fogFactor
    );
public:
    std::unique_ptr<ParticlesRenderer> particles;

    static bool showChunkBorders;
    static bool showEntitiesDebug;

    WorldRenderer(Engine* engine, LevelFrontend* frontend, Player* player);
    ~WorldRenderer();

    void draw(
        const DrawContext& context, 
        Camera& camera, 
        bool hudVisible,
        bool pause,
        float delta,
        PostProcessing* postProcessing
    );
    void drawBorders(int sx, int sy, int sz, int ex, int ey, int ez);

    /// @brief Render level without diegetic interface
    /// @param context graphics context
    /// @param camera active camera
    /// @param settings engine settings
    void renderLevel(
        const DrawContext& context, 
        const Camera& camera, 
        const EngineSettings& settings,
        float delta,
        bool pause
    );

    void clear();
};
