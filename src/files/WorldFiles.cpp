#include "WorldFiles.hpp"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <utility>

#include "../coders/byte_utils.hpp"
#include "../coders/json.hpp"
#include "../constants.hpp"
#include "../content/Content.hpp"
#include "../core_defs.hpp"
#include "../data/dynamic.hpp"
#include "../debug/Logger.hpp"
#include "../items/Inventory.hpp"
#include "../items/ItemDef.hpp"
#include "../lighting/Lightmap.hpp"
#include "../maths/voxmaths.hpp"
#include "../objects/EntityDef.hpp"
#include "../objects/Player.hpp"
#include "../physics/Hitbox.hpp"
#include "../settings.hpp"
#include "../typedefs.hpp"
#include "../util/data_io.hpp"
#include "../voxels/Block.hpp"
#include "../voxels/Chunk.hpp"
#include "../voxels/voxel.hpp"
#include "../window/Camera.hpp"
#include "../world/World.hpp"

#define WORLD_FORMAT_MAGIC ".VOXWLD"

static debug::Logger logger("world-files");

WorldFiles::WorldFiles(const fs::path& directory)
    : directory(directory), regions(directory) {
}

WorldFiles::WorldFiles(const fs::path& directory, const DebugSettings& settings)
    : WorldFiles(directory) {
    generatorTestMode = settings.generatorTestMode.get();
    doWriteLights = settings.doWriteLights.get();
    regions.generatorTestMode = generatorTestMode;
    regions.doWriteLights = doWriteLights;
}

WorldFiles::~WorldFiles() = default;

void WorldFiles::createDirectories() {
    fs::create_directories(directory / fs::path("data"));
    fs::create_directories(directory / fs::path("content"));
}

fs::path WorldFiles::getPlayerFile() const {
    return directory / fs::path("player.json");
}

fs::path WorldFiles::getResourcesFile() const {
    return directory / fs::path("resources.json");
}

fs::path WorldFiles::getWorldFile() const {
    return directory / fs::path(WORLD_FILE);
}

fs::path WorldFiles::getIndicesFile() const {
    return directory / fs::path("indices.json");
}

fs::path WorldFiles::getPacksFile() const {
    return directory / fs::path("packs.list");
}

void WorldFiles::write(const World* world, const Content* content) {
    if (world) {
        writeWorldInfo(world);
        if (!fs::exists(getPacksFile())) {
            writePacks(world->getPacks());
        }
    }
    if (generatorTestMode) {
        return;
    }

    writeIndices(content->getIndices());
    regions.write();
}

void WorldFiles::writePacks(const std::vector<ContentPack>& packs) {
    auto packsFile = getPacksFile();
    std::stringstream ss;
    ss << "# autogenerated; do not modify\n";
    for (const auto& pack : packs) {
        ss << pack.id << "\n";
    }
    files::write_string(packsFile, ss.str());
}

template <class T>
static void write_indices(
    const ContentUnitIndices<T>& indices, dynamic::List& list
) {
    size_t count = indices.count();
    for (size_t i = 0; i < count; i++) {
        list.put(indices.get(i)->name); //FIXME: .get(i) potential null pointer //-V522
    }
}

void WorldFiles::writeIndices(const ContentIndices* indices) {
    dynamic::Map root;
    write_indices(indices->blocks, root.putList("blocks"));
    write_indices(indices->items, root.putList("items"));
    write_indices(indices->entities, root.putList("entities"));
    files::write_json(getIndicesFile(), &root);
}

void WorldFiles::writeWorldInfo(const World* world) {
    files::write_json(getWorldFile(), world->serialize().get());
}

bool WorldFiles::readWorldInfo(World* world) {
    fs::path file = getWorldFile();
    if (!fs::is_regular_file(file)) {
        logger.warning() << "world.json does not exists";
        return false;
    }
    auto root = files::read_json(file);
    world->deserialize(root.get());
    return true;
}

static void read_resources_data(
    const Content* content, const dynamic::List_sptr& list, ResourceType type
) {
    const auto& indices = content->getIndices(type);
    for (size_t i = 0; i < list->size(); i++) {
        auto map = list->map(i);
        std::string name;
        map->str("name", name);
        size_t index = indices.indexOf(name);
        if (index == ResourceIndices::MISSING) {
            logger.warning() << "discard " << name;
        } else {
            indices.saveData(index, map->map("saved"));
        }
    }
}

bool WorldFiles::readResourcesData(const Content* content) {
    fs::path file = getResourcesFile();
    if (!fs::is_regular_file(file)) {
        logger.warning() << "resources.json does not exists";
        return false;
    }
    auto root = files::read_json(file);
    for (const auto& [key, _] : root->values) {
        if (auto resType = ResourceType_from(key)) {
            if (auto arr = root->list(key)) {
                read_resources_data(content, arr, *resType);
            }
        } else {
            logger.warning() << "unknown resource type: " << key;
        }
    }
    return true;
}

static void erase_pack_indices(dynamic::Map* root, const std::string& id) {
    auto prefix = id + ":";
    auto blocks = root->list("blocks");
    for (uint i = 0; i < blocks->size(); i++) {
        auto name = blocks->str(i);
        if (name.find(prefix) != 0) continue;
        auto value = blocks->getValueWriteable(i);
        *value = CORE_AIR;
    }

    auto items = root->list("items");
    for (uint i = 0; i < items->size(); i++) {
        auto name = items->str(i);
        if (name.find(prefix) != 0) continue;
        auto value = items->getValueWriteable(i);
        *value = CORE_EMPTY;
    }
}

void WorldFiles::removeIndices(const std::vector<std::string>& packs) {
    auto root = files::read_json(getIndicesFile());
    for (const auto& id : packs) {
        erase_pack_indices(root.get(), id);
    }
    files::write_json(getIndicesFile(), root.get());
}

fs::path WorldFiles::getFolder() const {
    return directory;
}
