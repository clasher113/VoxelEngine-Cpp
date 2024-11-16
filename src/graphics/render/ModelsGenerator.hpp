#pragma once

#include "graphics/commons/Model.hpp"
#include "maths/aabb.hpp"
#include "data/dv.hpp"

struct ItemDef;
class Assets;
class Content;
struct Block;

class ModelsGenerator {
public:
    static model::Model generate(
        const ItemDef& def, const Content& content, const Assets& assets
    );

    static model::Model fromCustom(
        const Assets& assets,
        const std::vector<AABB>& modelBoxes,
        const std::vector<std::string>& modelTextures,
        const std::vector<glm::vec3>& points,
        bool lighting
    );

    static model::Model loadCustomBlockModel(
        const dv::value& primitives, const Assets& assets, bool lighting
    );
};
