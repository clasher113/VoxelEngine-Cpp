#include "LevelController.hpp"
#include "../files/WorldFiles.hpp"
#include "../debug/Logger.hpp"
#include "../world/Level.hpp"
#include "../world/World.hpp"
#include "../physics/Hitbox.hpp"

#include "scripting/scripting.hpp"
#include "../interfaces/Object.hpp"

static debug::Logger logger("level-control");

LevelController::LevelController(EngineSettings& settings, std::unique_ptr<Level> level) 
  : settings(settings), level(std::move(level)),
    blocks(std::make_unique<BlocksController>(this->level.get(), settings.chunks.padding.get())),
    chunks(std::make_unique<ChunksController>(this->level.get(), settings.chunks.padding.get())),
    player(std::make_unique<PlayerController>(this->level.get(), settings, blocks.get())) {

    scripting::on_world_load(this);
}

void LevelController::update(float delta, bool input, bool pause) {
    player->update(delta, input, pause);
    glm::vec3 position = player->getPlayer()->hitbox->position;
    level->loadMatrix(position.x, position.z, 
        settings.chunks.loadDistance.get() + 
        settings.chunks.padding.get() * 2);
    chunks->update(settings.chunks.loadSpeed.get());

    // erease null pointers
    level->objects.erase(
        std::remove_if(
            level->objects.begin(), level->objects.end(),
            [](auto obj) { return obj == nullptr; }),
        level->objects.end()
    );
    
    if (!pause) {
        // update all objects that needed
        for (auto obj : level->objects) {
            if (obj && obj->shouldUpdate) {
                obj->update(delta);
            }
        }
        blocks->update(delta);
    }
}

void LevelController::saveWorld() {
    level->getWorld()->wfile->createDirectories();
    logger.info() << "writing world";
    scripting::on_world_save();
    level->getWorld()->write(level.get());
}

void LevelController::onWorldQuit() {
    scripting::on_world_quit();
}

Level* LevelController::getLevel() {
    return level.get();
}

Player* LevelController::getPlayer() {
    return player->getPlayer();
}

BlocksController* LevelController::getBlocksController() {
    return blocks.get();
}

PlayerController* LevelController::getPlayerController() {
    return player.get();
}
