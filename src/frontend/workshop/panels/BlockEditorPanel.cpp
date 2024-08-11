#include "../WorkshopScreen.hpp"

#include "../../../items/ItemDef.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopPreview.hpp"
#include "../WorkshopSerializer.hpp"
#include "../../../content/Content.hpp"
#include "../BlockModelConverter.hpp"

void workshop::WorkShopScreen::createBlockEditor(Block& block) {
	createPanel([this, &block]() {
		validateBlock(assets, block);

		std::string actualName(block.name.substr(currentPack.id.size() + 1));

		auto panel = std::make_shared<gui::Panel>(glm::vec2(200));

		panel->add(std::make_shared<gui::Label>(actualName));

		panel->add(std::make_shared<gui::Label>("Caption"));
		createTextBox(panel, block.caption, L"Example block");

		createFullCheckBox(panel, L"Light passing", block.lightPassing, L"Does the block passing lights into itself");
		createFullCheckBox(panel, L"Sky light passing", block.skyLightPassing, L"Does the block passing top-down sky lights into itself");
		createFullCheckBox(panel, L"Obstacle", block.obstacle, L"Is the block a physical obstacle");
		createFullCheckBox(panel, L"Selectable", block.selectable, L"Can the block be selected");
		createFullCheckBox(panel, L"Replaceable", block.replaceable, L"Can the block be replaced with other. \n Examples of replaceable blocks: air, flower, water");
		createFullCheckBox(panel, L"Shadeless", block.shadeless, L"Does block model have shading");
		createFullCheckBox(panel, L"Ambient Occlusion", block.ambientOcclusion, L"Does block model have vertex-based AO effect");
		createFullCheckBox(panel, L"Breakable", block.breakable, L"Can player destroy the block");
		createFullCheckBox(panel, L"Grounded", block.grounded, L"Can the block exist without physical support be a solid block below");
		createFullCheckBox(panel, L"Hidden", block.hidden, L"Turns off block item generation");

		panel->add(std::make_shared<gui::Label>(L"Picking item"));
		auto pickingItemPanel = std::make_shared<gui::Panel>(glm::vec2(panel->getSize().x, 35.f));
		auto item = [this](std::string pickingItem) {
			return (content->items.find(pickingItem) == nullptr ? content->items.find("core:empty") : content->items.find(pickingItem));
		};
		auto atlas = [this, item, &block](std::string pickingItem) {
			ItemDef* i = item(pickingItem);
			if (i->iconType == item_icon_type::block) return previewAtlas;
			return getAtlas(assets, i->icon);
		};
		auto texName = [](ItemDef* item) {
			if (item->iconType == item_icon_type::none) return std::string("transparent");
			if (item->iconType == item_icon_type::block) return item->icon;
			return getTexName(item->icon);
		};
		auto rbutton = std::make_shared<gui::IconButton>(glm::vec2(panel->getSize().x, 35.f), texName(item(block.pickingItem)), atlas(block.pickingItem),
			texName(item(block.pickingItem)));
		rbutton->listenAction([this, rbutton, panel, texName, item, atlas, &block](gui::GUI*) {
			createContentList(DefType::ITEM, 5, true, [this, rbutton, texName, item, atlas, &block](std::string name) {
				block.pickingItem = name;
				rbutton->setIcon(atlas(block.pickingItem), texName(item(block.pickingItem)));
				rbutton->setText(item(block.pickingItem)->name);
				removePanels(5);
			}, panel->calcPos().x + panel->getSize().x);
		});
		panel->add(rbutton);

		auto texturePanel = std::make_shared<gui::Panel>(glm::vec2(panel->getSize().x, 35.f));
		texturePanel->setColor(glm::vec4(0.f));
		panel->add(texturePanel);

		auto processModelChange = [this, texturePanel, panel](Block& block) {
			clearRemoveList(texturePanel);
			if (block.model == BlockModel::custom) {
				createCustomModelEditor(block, 0, PrimitiveType::AABB);
				texturePanel->add(removeList.emplace_back(std::make_shared<gui::Button>(L"Import model", glm::vec4(10), [this, panel, &block](gui::GUI*){
					createBlockConverterPanel(block, panel->getPos().x + panel->getSize().x);
				})));
			}
			else {
				createCustomModelEditor(block, 0, PrimitiveType::HITBOX);
				createTexturesPanel(texturePanel, 35.f, block.textureFaces, block.model);
			}
			texturePanel->cropToContent();
			preview->updateMesh();
		};

		auto button = std::make_shared<gui::Button>(L"Model: " + util::str2wstr_utf8(to_string(block.model)), glm::vec4(10.f), gui::onaction());
		button->listenAction([&block, button, processModelChange](gui::GUI*) {
			BlockModel model = incrementEnumClass(block.model, 1);
			if (model > BlockModel::custom) model = BlockModel::none;
			block.model = model;
			button->setText(L"Model: " + util::str2wstr_utf8(to_string(block.model)));
			processModelChange(block);
		});
		panel->add(button);

		processModelChange(block);

		auto getRotationName = [](const Block& block) {
			if (!block.rotatable) return "none";
			return block.rotations.name.data();
		};

		button = std::make_shared<gui::Button>(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)), glm::vec4(10.f), gui::onaction());
		button->listenAction([&block, getRotationName, button](gui::GUI*) {
			std::string rot = getRotationName(block);
			block.rotatable = true;
			if (rot == "none") {
				block.rotations = BlockRotProfile::PANE;
			}
			else if (rot == "pane") {
				block.rotations = BlockRotProfile::PIPE;
			}
			else if (rot == "pipe") {
				block.rotatable = false;
			}
			button->setText(L"Rotation: " + util::str2wstr_utf8(getRotationName(block)));
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("Draw group (0 - 255)"));
		panel->add(createNumTextBox<ubyte>(block.drawGroup, L"0", 0, 255));
		createEmissionPanel(panel, block.emission);

		panel->add(std::make_shared<gui::Label>("Block size (0 - 5)"));
		panel->add(createVectorPanel(block.size, glm::i8vec3(0), glm::i8vec3(5), panel->getSize().x, 1, [processModelChange, &block](){
			processModelChange(block);
		}));

		panel->add(std::make_shared<gui::Label>("Script file"));
		button = std::make_shared<gui::Button>(util::str2wstr_utf8(getScriptName(currentPack, block.scriptName)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, panel, button, actualName, &block](gui::GUI*) {
			createScriptList(5, panel->calcPos().x + panel->getSize().x, [this, button, actualName, &block](const std::string& string) {
				removePanels(5);
				std::string scriptName(getScriptName(currentPack, string));
				block.scriptName = (scriptName == NOT_SET ? (getScriptName(currentPack, actualName) == NOT_SET ? actualName : "") : scriptName);
				button->setText(util::str2wstr_utf8(scriptName));
			});
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("Block material"));
		button = std::make_shared<gui::Button>(util::str2wstr_utf8(block.material), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, panel, button, &block](gui::GUI*) {
			createMaterialsList(true, 5, panel->calcPos().x + panel->getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				button->setText(util::str2wstr_utf8(block.material = string));
			});
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("UI layout"));
		button = std::make_shared<gui::Button>(util::str2wstr_utf8(getUILayoutName(assets, block.uiLayout)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, panel, button, &block](gui::GUI*) {
			createUILayoutList(true, 5, panel->calcPos().x + panel->getSize().x, [this, button, &block](const std::string& string) {
				removePanels(5);
				std::string layoutName(getUILayoutName(assets, string));
				block.uiLayout = (layoutName == NOT_SET ? (getUILayoutName(assets, block.name) == NOT_SET ? block.name : "") : layoutName);
				button->setText(util::str2wstr_utf8(layoutName));
			});
		});
		panel->add(button);

		panel->add(std::make_shared<gui::Label>("Inventory size"));
		panel->add(createNumTextBox<uint>(block.inventorySize, L"0 - no inventory", 0, std::numeric_limits<decltype(block.inventorySize)>::max()));

		panel->add(std::make_shared<gui::Label>("Tick interval (1 - 20)"));
		panel->add(createNumTextBox<uint>(block.tickInterval, L"1 - 20tps, 2 - 10tps", 1, 20));

		panel->add(std::make_shared<gui::Button>(L"Save", glm::vec4(10.f), [this, &block, actualName](gui::GUI*) {
			saveBlock(block, currentPack.folder, actualName);
		}));
		panel->add(std::make_shared<gui::Button>(L"Rename", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(DefAction::RENAME, DefType::BLOCK, actualName);
		}));
		panel->add(std::make_shared<gui::Button>(L"Delete", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(DefAction::DELETE, DefType::BLOCK, actualName);
		}));

		return panel;
	}, 2);
}