#include "WorkshopSerializer.hpp"

#include "coders/json.hpp"
#include "coders/xml.hpp"
#include "content/ContentPack.hpp"
#include "data/dv.hpp"
#include "files/files.hpp"
#include "items/ItemDef.hpp"
#include "objects/EntityDef.hpp"
#include "voxels/Block.hpp"
#include "WorkshopUtils.hpp"
#include "constants.hpp"
#include "util/stringutil.hpp"
#include "objects/rigging.hpp"

#include <algorithm>
#include <map>

static void putVec3(dv::value& list, const glm::vec3& vector){
	list.add(vector.x); list.add(vector.y); list.add(vector.z);
}

static void putAABB(dv::value& list, AABB aabb){
	aabb.b -= aabb.a;
	putVec3(list, aabb.a); putVec3(list, aabb.b);
}

static std::string to_string(const glm::vec3& vector, char delimiter = ','){
	return std::to_string(vector.x) + delimiter + std::to_string(vector.y) + delimiter + std::to_string(vector.z);
}

std::string workshop::stringify(const dv::value& root, bool nice) {
	json::precision = 5;
	std::string result = json::stringify(root, nice, "  ");
	json::precision = 15;

	return result;
}

dv::value workshop::toJson(const Block& block, const std::string& actualName, const Block* const parent, const std::string& newParent) {
	Block temp(block.name);
	const Block& master = (parent ? *parent : temp);

	dv::value root = dv::object();

	if (block.model != BlockModel::custom) {
		//if (!std::equal(block.textureFaces->begin(), block.textureFaces->end(), master.textureFaces->begin(), master.textureFaces->end())) {
		//	if (std::all_of(std::cbegin(block.textureFaces), std::cend(block.textureFaces), [&r = block.textureFaces[0]](const std::string& value) {return value == r; })) {
		//		root["texture"] = block.textureFaces[0];
		//	}
		//	else {
		//		dv::value& texarr = root.list("texture-faces");
		//		for (size_t i = 0; i < 6; i++) {
		//			texarr.add(block.textureFaces[i]);
		//		}
		//	}
		//}
	}
#define NOT_EQUAL(MEMBER) master.MEMBER != block.MEMBER
	
	if (!newParent.empty()) root["parent"] = newParent;
	if (NOT_EQUAL(ambientOcclusion)) root["ambient-occlusion"] = block.ambientOcclusion;
	if (NOT_EQUAL(lightPassing)) root["light-passing"] = block.lightPassing;
	if (NOT_EQUAL(skyLightPassing)) root["sky-light-passing"] = block.skyLightPassing;
	if (NOT_EQUAL(obstacle)) root["obstacle"] = block.obstacle;
	if (NOT_EQUAL(selectable)) root["selectable"] = block.selectable;
	if (NOT_EQUAL(replaceable)) root["replaceable"] = block.replaceable;
	if (NOT_EQUAL(breakable)) root["breakable"] = block.breakable;
	if (NOT_EQUAL(grounded)) root["grounded"] = block.grounded;
	if (NOT_EQUAL(drawGroup)) root["draw-group"] = block.drawGroup;
	if (NOT_EQUAL(hidden)) root["hidden"] = block.hidden;
	if (NOT_EQUAL(shadeless)) root["shadeless"] = block.shadeless;
	if (NOT_EQUAL(tickInterval)) root["tick-interval"] = block.tickInterval;
	if (NOT_EQUAL(inventorySize)) root["inventory-size"] = block.inventorySize;
	if (NOT_EQUAL(model)) root["model"] = to_string(block.model);
	if ((NOT_EQUAL(rotatable)) || NOT_EQUAL(rotations.name)) root["rotation"] = block.rotatable ? block.rotations.name : "none";
	if (NOT_EQUAL(pickingItem)) root["picking-item"] = block.pickingItem;
	if (NOT_EQUAL(scriptName)) root["script-name"] = block.scriptName;
	if (NOT_EQUAL(material)) root["material"] = block.material;
	if (NOT_EQUAL(uiLayout)) root["ui-layout"] = block.uiLayout;
	
	if (!std::equal(std::begin(master.emission), std::end(master.emission), std::begin(block.emission))) {
		dv::value& emissionarr = root.list("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.add(block.emission[i]);
		}
	}

	if (NOT_EQUAL(size)) {
		dv::value& sizeList = root.list("size");
		sizeList.multiline = false;
		putVec3(sizeList, block.size);
	}

	if (!std::equal(block.hitboxes.begin(), block.hitboxes.end(), master.hitboxes.begin(), master.hitboxes.end(), [](const AABB& hitbox, const AABB& modelBox) {
		return hitbox == modelBox;
	})) {
		if (block.hitboxes.size() == 1) {
			const AABB& hitbox = block.hitboxes.front();
			AABB aabb;
			if (block.model == BlockModel::custom || !(aabb == hitbox)) {
				dv::value& boxarr = root.list("hitbox");
				boxarr.multiline = false;
				putAABB(boxarr, hitbox);
			}
		}
		else if (!block.hitboxes.empty()) {
			//if (!std::equal(block.hitboxes.begin(), block.hitboxes.end(), block.modelBoxes.begin(), block.modelBoxes.end(), [](const AABB& hitbox, const AABB& modelBox) {
			//	return hitbox == modelBox;
			//})) {
			//	dv::value& hitboxesArr = root.list("hitboxes");
			//	for (const auto& hitbox : block.hitboxes) {
			//		dv::value& hitboxArr = hitboxesArr.list();
			//		hitboxArr.multiline = false;
			//		putAABB(hitboxArr, hitbox);
			//	}
			//}
		}
	}

	auto isElementsEqual = [](const std::vector<std::string>& arr, size_t offset, size_t numElements) {
		return std::all_of(std::cbegin(arr) + offset, std::cbegin(arr) + offset + numElements, [&r = arr[offset]](const std::string& value) {return value == r; });
	};
	if (block.model == BlockModel::custom) {
		dv::value& model = root.object("model-primitives");
		size_t textureOffset = 0;
		//const size_t parentTexSize = parent ? parent->modelTextures.size() : 0;
		//if (!block.modelBoxes.empty()) {
		//	const size_t parentPrimitiveSize = parent ? parent->modelBoxes.size() : 0;
		//	if (parentPrimitiveSize != block.modelBoxes.size()) {
		//		dv::value& aabbs = model.list("aabbs");
		//		for (size_t i = parentPrimitiveSize; i < block.modelBoxes.size(); i++) {
		//			dv::value& aabb = aabbs.list();
		//			aabb.multiline = false;
		//			putAABB(aabb, block.modelBoxes[i]);
		//			const size_t textures = (isElementsEqual(block.modelTextures, textureOffset, 6) ? 1 : 6);
		//			for (size_t j = 0; j < textures; j++) {
		//				aabb.add(block.modelTextures[textureOffset + j - parentTexSize]);
		//			}
		//			textureOffset += 6;
		//		}
		//	}
		//}
		//if (!block.modelExtraPoints.empty()) {
		//	const size_t parentPrimitiveSize = parent ? parent->modelBoxes.size() : 0;
		//	if (parentPrimitiveSize != block.modelExtraPoints.size()) {
		//		dv::value& tetragons = model.list("tetragons");
		//		for (size_t i = parentPrimitiveSize; i < block.modelExtraPoints.size(); i += 4) {
		//			dv::value& tetragon = tetragons.list();
		//			tetragon.multiline = false;
		//			putVec3(tetragon, block.modelExtraPoints[i]);
		//			putVec3(tetragon, block.modelExtraPoints[i + 1] - block.modelExtraPoints[i]);
		//			putVec3(tetragon, block.modelExtraPoints[i + 3] - block.modelExtraPoints[i]);
		//			tetragon.add(block.modelTextures[textureOffset - parentTexSize]);
		//			textureOffset++;
		//		}
		//	}
		//}
		if (model.size() == 0) root.erase("model-primitives");
	}

	return root;
#undef NOT_EQUAL
}

dv::value workshop::toJson(const ItemDef& item, const std::string& actualName, const ItemDef* const parent, const std::string& newParent) {
	ItemDef temp(item.name);
	const ItemDef& master = (parent ? *parent : temp);

#define NOT_EQUAL(MEMBER) master.MEMBER != item.MEMBER

	dv::value root = dv::object();

	if (!newParent.empty()) root["parent"] = newParent;
	if (NOT_EQUAL(stackSize)) root["stack-size"] = item.stackSize;
	if (NOT_EQUAL(iconType)) {
		std::string iconStr("");
		switch (item.iconType) {
			case ItemIconType::NONE: iconStr = "none";
				break;
			case ItemIconType::BLOCK: iconStr = "block";
				break;
			case ItemIconType::SPRITE: iconStr = "sprite";
				break;
		}
		if (!iconStr.empty()) root["icon-type"] = iconStr;
	}
	if (NOT_EQUAL(icon)) root["icon"] = item.icon;
	if (NOT_EQUAL(placingBlock)) root["placing-block"] = item.placingBlock;
	if (NOT_EQUAL(scriptName)) root["script-name"] = item.scriptName;

	if (!std::equal(std::begin(master.emission), std::end(master.emission), std::begin(item.emission))) {
		dv::value& emissionarr = root.list("emission");
		emissionarr.multiline = false;
		for (size_t i = 0; i < 3; i++) {
			emissionarr.add(item.emission[i]);
		}
	}

	return root;
#undef NOT_EQUAL
}

dv::value workshop::toJson(const EntityDef& entity, const std::string& actualName, const EntityDef* const parent, const std::string& newParent) {
	EntityDef temp(entity.name);
	const EntityDef& master = (parent ? *parent : temp);

#define NOT_EQUAL(MEMBER) master.MEMBER != entity.MEMBER

	dv::value root = dv::object();

	if (!newParent.empty()) root["parent"] = newParent;
	if (NOT_EQUAL(save.enabled)) root["save"] = entity.save.enabled;
	if (NOT_EQUAL(save.skeleton.pose)) root["save-skeleton-pose"] = entity.save.skeleton.pose;
	if (NOT_EQUAL(save.skeleton.textures)) root["save-skeleton-textures"] = entity.save.skeleton.textures;
	if (NOT_EQUAL(save.body.velocity)) root["save-body-velocity"] = entity.save.body.velocity;
	if (NOT_EQUAL(save.body.settings)) root["save-body-settings"] = entity.save.body.settings;
	if (NOT_EQUAL(skeletonName)) root["skeleton-name"] = entity.skeletonName;
	if (NOT_EQUAL(blocking)) root["blocking"] = entity.blocking;
	if (NOT_EQUAL(bodyType)) root["body-type"] = to_string(entity.bodyType);
	if (NOT_EQUAL(hitbox)) {
		dv::value& list = root.list("hitbox");
		list.multiline = false;
		putVec3(list, entity.hitbox);
	}
	if (NOT_EQUAL(components.size())) {
		dv::value& list = root.list("components");
		for (size_t i = parent ? parent->components.size() : 0; i < entity.components.size(); i++) {
			list.add(entity.components[i]);
		}
	}
	std::map<size_t, std::string> sensorsMap;
	if (NOT_EQUAL(boxSensors.size())){
		for (size_t i = parent ? parent->boxSensors.size() : 0; i < entity.boxSensors.size(); i++) {
			const auto& sensor = entity.boxSensors[i];
			sensorsMap.emplace(sensor.first, "aabb," + to_string(sensor.second.a) + ',' + to_string(sensor.second.b));
		}
	}
	if (NOT_EQUAL(radialSensors.size())){
		for (size_t i = parent ? parent->radialSensors.size() : 0; i < entity.radialSensors.size(); i++) {
			const auto& sensor = entity.radialSensors[i];
			sensorsMap.emplace(sensor.first, "radius," + std::to_string(sensor.second));
		}
	}
	if (!sensorsMap.empty()){
		dv::value& sensors = root.list("sensors");
		for (const auto& sensor : sensorsMap){
			auto split = util::split(sensor.second, ',');
			dv::value& list = sensors.list();
			list.multiline = false;
			list.add(split[0]);
			for (size_t i = 1; i < split.size(); i++) {
				list.add(stof(split[i]));
			}
		}
	}

	return root;
#undef NOT_EQUAL
}

static void putBone(dv::value& map, const rigging::Bone& bone){
	if (!bone.getName().empty()) map["name"] = bone.getName();
	if (!bone.model.name.empty()) map["model"] = bone.model.name;
	if (bone.getOffset() != glm::vec3(0.f)) {
		dv::value& list = map.list("offset");
		list.multiline = false;
		putVec3(list, bone.getOffset());
	}
	if (!bone.getSubnodes().empty()) {
		dv::value& list = map.list("nodes");
		for (const std::unique_ptr<rigging::Bone>& elem : bone.getSubnodes()) {
			dv::value& subMap = list.object();
			putBone(subMap, *elem);
		}
	}
}

dv::value workshop::toJson(const rigging::Bone& rootBone, const std::string& actualName) {
	dv::value root = dv::object();

	putBone(root["root"] = dv::object(), rootBone);

	return root;
}

void workshop::saveContentPack(const ContentPack& pack) {
	dv::value root = dv::object();
	if (!pack.id.empty()) root["id"] = pack.id;
	if (!pack.title.empty()) root["title"] = pack.title;
	if (!pack.version.empty()) root["version"] = pack.version;
	if (!pack.creator.empty()) root["creator"] = pack.creator;
	if (!pack.description.empty()) root["description"] = pack.description;

	if (!pack.dependencies.empty()) {
		dv::value& dependencies = root.list("dependencies");
		for (const auto& elem : pack.dependencies) {
			if (!elem.id.empty()) dependencies.add(elem.id);
		}
		if (dependencies.size() == 0) root.erase("dependencies");
	}
	files::write_json(pack.folder / ContentPack::PACKAGE_FILENAME, root);
}

void workshop::saveBlock(const Block& block, const fs::path& packFolder, const std::string& actualName, const Block* const parent, const std::string& newParent) {
	fs::path path = packFolder / ContentPack::BLOCKS_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_string(path / fs::path(actualName + ".json"), stringify(toJson(block, actualName, parent, newParent), true));
}

void workshop::saveItem(const ItemDef& item, const fs::path& packFolder, const std::string& actualName, const ItemDef* const parent, const std::string& newParent) {
	fs::path path = packFolder / ContentPack::ITEMS_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_string(path / fs::path(actualName + ".json"), stringify(toJson(item, actualName, parent, newParent), true));
}

void workshop::saveEntity(const EntityDef& entity, const std::filesystem::path& packFolder, const std::string& actualName, const EntityDef* const parent, const std::string& newParent) {
	fs::path path = packFolder / ContentPack::ENTITIES_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_string(path / fs::path(actualName + ".json"), stringify(toJson(entity, actualName, parent, newParent), true));
}

void workshop::saveSkeleton(const rigging::Bone& root, const std::filesystem::path& packFolder, const std::string& actualName) {
	fs::path path = packFolder / SKELETONS_FOLDER;
	if (!fs::is_directory(path)) fs::create_directories(path);
	files::write_string(path / fs::path(actualName + ".json"), stringify(toJson(root, actualName), true));
}

void workshop::saveDocument(std::shared_ptr<xml::Document> document, const fs::path& packFolder, const std::string& actualName) {
	std::ofstream os;
	os.open(packFolder / LAYOUTS_FOLDER / (actualName + ".xml"));
	os << xml::stringify(document);
	os.close();
}
