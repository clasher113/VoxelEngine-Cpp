#pragma once

#include <vector>
#include <memory>
#include <unordered_map>

#include "Emitter.hpp"
#include "typedefs.hpp"

class Texture;
class Assets;
class Camera;
class MainBatch;
class Level;
struct GraphicsSettings;

class ParticlesRenderer {
    const Level& level;
    const Assets& assets;
    const GraphicsSettings* settings;
    std::unordered_map<const Texture*, std::vector<Particle>> particles;
    std::unique_ptr<MainBatch> batch;

    std::unordered_map<u64id_t, std::unique_ptr<Emitter>> emitters;
    u64id_t nextEmitter = 1;

    void renderParticles(const Camera& camera, float delta);
public:
    ParticlesRenderer(
        const Assets& assets,
        const Level& level,
        const GraphicsSettings* settings
    );
    ~ParticlesRenderer();

    void render(const Camera& camera, float delta);

    u64id_t add(std::unique_ptr<Emitter> emitter);

    /// @brief Perform garbage collection (remove extra dead emitters).
    /// @note Emitters are deleting without GC when there's no particles with same
    /// texture left.
    /// @note Currently unused
    void gc();

    /// @brief Get emitter by UID
    /// @return Emitter or nullptr
    Emitter* getEmitter(u64id_t id) const;

    static size_t visibleParticles;
    static size_t aliveEmitters;
};
