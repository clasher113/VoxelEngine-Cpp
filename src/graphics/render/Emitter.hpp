#pragma once

#include <vector>
#include <variant>
#include <glm/glm.hpp>

#include "typedefs.hpp"

#include "maths/UVRegion.hpp"
#include "maths/util.hpp"
#include "presets/ParticlesPreset.hpp"

class Level;
class Emitter;

struct Particle {
    /// @brief Pointer used to access common behaviour.
    /// Emitter must be utilized after all related particles despawn.
    Emitter* emitter;
    /// @brief Some random integer for visuals configuration.
    int random;
    /// @brief Global position
    glm::vec3 position;
    /// @brief Linear velocity
    glm::vec3 velocity;
    /// @brief Remaining life time
    float lifetime;
    /// @brief UV region
    UVRegion region;
};

class Texture;

using EmitterOrigin = std::variant<glm::vec3, entityid_t>;

class Emitter {
    const Level& level;
    /// @brief Static position or entity
    EmitterOrigin origin;
    /// @brief Particle prototype
    Particle prototype;
    /// @brief Particle
    const Texture* texture;
    /// @brief Number of particles should be spawned before emitter deactivation.
    /// -1 is infinite.
    int count;
    /// @brief Spawn timer used to determine number of particles 
    /// to spawn on update. May be innacurate.
    float timer = 0.0f;

    util::PseudoRandom random;
public:
    ParticlesPreset preset;

    Emitter(
        const Level& level,
        std::variant<glm::vec3, entityid_t> origin,
        ParticlesPreset preset,
        const Texture* texture,
        const UVRegion& region,
        int count
    );

    explicit Emitter(const Emitter&) = delete;

    /// @return Emitter particles texture
    const Texture* getTexture() const;

    /// @brief Update emitter and spawn particles
    /// @param delta delta time
    /// @param cameraPosition current camera global position
    /// @param particles destination particles vector
    void update(
        float delta,
        const glm::vec3& cameraPosition,
        std::vector<Particle>& particles
    );

    /// @brief Set remaining particles count to 0
    void stop();

    /// @return true if the emitter has spawned all particles
    bool isDead() const;

    const EmitterOrigin& getOrigin() const;

    void setOrigin(const EmitterOrigin& origin);
};
