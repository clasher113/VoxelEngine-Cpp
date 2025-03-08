#include "../WorkshopScreen.hpp"

#include "items/ItemDef.hpp"
#include "content/Content.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopSerializer.hpp"
#include "core_defs.hpp"

void workshop::WorkShopScreen::createItemEditor(ItemDef& item) {
	createPanel([this, &item]() {
		std::string actualName(item.name.substr(currentPackId.size() + 1));
		fs::path filePath(currentPack.folder / ContentPack::ITEMS_FOLDER / std::string(actualName + ".json"));
		const bool hasFile = fs::is_regular_file(filePath);

		gui::Panel& panel = *new gui::Panel(glm::vec2(settings.itemEditorWidth));
		if (!hasFile) {
			gui::Label& label = *new gui::Label(L"Autogenerated item from existing block\nCreate file to edit item propreties");
			label.setTextWrapping(true);
			label.setMultiline(true);
			label.setSize(panel.getSize());
			panel << label;

			panel << new gui::Button(L"Create file", glm::vec4(10.f), [this, &item, actualName](gui::GUI*) {
				saveItem(item, currentPack.folder, actualName, nullptr, "");
				createItemEditor(item);
			});
			return std::ref(panel);
		}
		panel << new gui::Label(actualName);

		panel << new gui::Label("Caption");
		createTextBox(panel, item.caption, L"Example item");

		panel << new gui::Label(L"Parent item");
		auto parentIcoName = [](const ItemDef* const parentItem) {
			if (parentItem) return parentItem->name;
			return NOT_SET;
		};
		auto parentIcoTexName = [parentIcoName](const ItemDef* const parent) {
			return parentIcoName(parent) == NOT_SET ? CORE_AIR : (parent->iconType == ItemIconType::BLOCK ? parent->icon : getTexName(parent->icon));
		};
		auto parentIcoAtlas = [this, parentIcoName](const ItemDef* const parent) {
			return parentIcoName(parent) == NOT_SET ? previewAtlas : (parent->iconType == ItemIconType::BLOCK ? previewAtlas : getAtlas(assets, parent->icon));
		};

		BackupData& backupData = itemsList[actualName];
		const ItemDef* currentParent = content->items.find(backupData.currentParent);
		const ItemDef* newParent = content->items.find(backupData.newParent);

		gui::IconButton& parentItem = *new gui::IconButton(glm::vec2(panel.getSize().x, 35.f), parentIcoName(newParent), parentIcoAtlas(newParent), parentIcoTexName(newParent));
		parentItem.listenAction([=, &panel, &backupData, &parentItem, &item](gui::GUI*) {
			createContentList(ContentType::ITEM, true, 5, panel.calcPos().x + panel.getSize().x, [=, &backupData, &parentItem, &item](const std::string& string) {
				const ItemDef* parent = content->items.find(string);
				if (parent->name == CORE_EMPTY || parent->name == item.name) parent = nullptr;
				backupData.newParent = parent ? string : "";
				parentItem.setIcon(parentIcoAtlas(parent), parentIcoTexName(parent));
				parentItem.setText(parentIcoName(parent));
				removePanels(5);
			});
		});
		panel << parentItem;

		panel << new gui::Label(L"Stack size:");
		panel << createNumTextBox<uint32_t>(item.stackSize, L"1", 1, 64);
		createEmissionPanel(panel, item.emission);

		panel << new gui::Label(L"Icon");
		gui::Panel& textureIco = *new gui::Panel(glm::vec2(panel.getSize().x, 35.f));
		createTexturesPanel(textureIco, 35.f, item.icon, item.iconType);
		panel << textureIco;

		const wchar_t* const iconTypes[] = { L"none", L"sprite", L"block" };
		gui::Button* button = new gui::Button(L"Icon type: " + std::wstring(iconTypes[static_cast<unsigned int>(item.iconType)]), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, button, iconTypes, &item, &textureIco, &panel](gui::GUI*) {
			const char* const icons[] = { CORE_AIR.c_str(), "blocks:notfound", CORE_AIR.c_str() };
			item.iconType = incrementEnumClass(item.iconType, 1);
			if (item.iconType > ItemIconType::BLOCK) item.iconType = ItemIconType::NONE;
			item.icon = icons[static_cast<size_t>(item.iconType)];
			removePanels(3);
			button->setText(L"Icon type: " + std::wstring(iconTypes[static_cast<unsigned int>(item.iconType)]));
			removeRemovable(textureIco);
			createTexturesPanel(textureIco, 35.f, item.icon, item.iconType);
			textureIco.cropToContent();
			panel.refresh();
		});
		panel << button;

		panel << new gui::Label(L"Placing block");
		gui::Panel& placingBlockPanel = *new gui::Panel(glm::vec2(panel.getSize().x, 35.f));
		createTexturesPanel(placingBlockPanel, 35.f, item.placingBlock, ItemIconType::BLOCK);
		panel << placingBlockPanel;

		panel << new gui::Label("Script file");
		button = new gui::Button(util::str2wstr_utf8(getScriptName(currentPack, item.scriptName)), glm::vec4(10.f), gui::onaction());
		button->listenAction([this, &panel, button, actualName, &item](gui::GUI*) {
			createScriptList(5, panel.calcPos().x + panel.getSize().x, [this, button, actualName, &item](const std::string& string) {
				removePanels(5);
				std::string scriptName(getScriptName(currentPack, string));
				item.scriptName = (scriptName == NOT_SET ? (getScriptName(currentPack, actualName) == NOT_SET ? actualName : "") : scriptName);
				button->setText(util::str2wstr_utf8(scriptName));
			});
		});
		panel << button;

		panel << new gui::Button(L"Save", glm::vec4(10.f), [this, actualName, &item, &backupData, currentParent](gui::GUI*) {
			backupData.string = stringify(toJson(item, actualName, currentParent, backupData.newParent), false);
			saveItem(item, currentPack.folder, actualName, currentParent, backupData.newParent);
		});
		if (!item.generated) {
			panel << new gui::Button(L"Rename", glm::vec4(10.f), [this, actualName](gui::GUI*) {
				createDefActionPanel(ContentAction::RENAME, ContentType::ITEM, actualName);
			});
		}
		panel << new gui::Button(L"Delete", glm::vec4(10.f), [this, &item, actualName](gui::GUI*) {
			createDefActionPanel(ContentAction::DELETE, ContentType::ITEM, actualName, !item.generated);
		});

		return std::ref(panel);
	}, 2);
}