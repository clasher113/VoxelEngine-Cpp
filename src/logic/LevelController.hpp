#pragma once

#include <memory>

#include "BlocksController.hpp"
#include "ChunksController.hpp"
#include "util/Clock.hpp"

class Engine;
class Level;
class Player;
struct EngineSettings;

/// @brief LevelController manages other controllers
class LevelController {
    EngineSettings& settings;
    std::unique_ptr<Level> level;
    // Sub-controllers
    std::unique_ptr<BlocksController> blocks;
    std::unique_ptr<ChunksController> chunks;

    util::Clock playerTickClock;
public:
    LevelController(Engine* engine, std::unique_ptr<Level> level, Player* clientPlayer);

    /// @param delta time elapsed since the last update
    /// @param pause is world and player simulation paused
    void update(float delta, bool pause);

    void saveWorld();

    void onWorldQuit();

    Level* getLevel();

    BlocksController* getBlocksController();
    ChunksController* getChunksController();
};
