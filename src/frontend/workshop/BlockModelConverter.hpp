#ifndef FRONTEND_MENU_BLOCK_MODEL_CONVERTER_HPP
#define FRONTEND_MENU_BLOCK_MODEL_CONVERTER_HPP

#include "../../maths/aabb.hpp"
#include "../../maths/UVRegion.hpp"
#include "WorkshopUtils.hpp"

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <glm/glm.hpp>

class Block;
class Atlas;
struct ContentPack;
namespace dynamic{
	class Map;
}
namespace gui{
	class UINode;
}

namespace workshop {
	class BlockModelConverter {
	public:
		BlockModelConverter(const std::filesystem::path& filePath);

		size_t prepareTextures();
		std::unordered_map<std::string, std::string>& getTextureMap() { return textureList; };

		Block* convert(const ContentPack& currentPack, Atlas* blocksAtlas, const std::string& blockName);
	private:
		struct TextureData {
			int rotation = 0;
			UVRegion uv;
			std::string name;
			bool isUnique() const { return rotation != 0 || !(uv == UVRegion()); }
		};
		struct PrimitiveData {
			glm::vec3 rotation{ 0.f };
			glm::vec3 axis{ 0.f };
			glm::vec3 origin{ 0.f };
			AABB aabb;
			TextureData textures[6];
		};

		std::vector<PrimitiveData> primitives;
		std::vector<std::string> preparedTextures;
		std::unordered_map<std::string, std::string> textureList;
		std::unordered_map<std::string, std::vector<TextureData>> croppedTextures;
	};
}
#endif // !FRONTEND_MENU_BLOCK_MODEL_CONVERTER_HPP