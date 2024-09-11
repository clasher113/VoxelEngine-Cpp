#include "WorkshopScreen.hpp"

#include "../../audio/audio.hpp"
#include "../../coders/png.hpp"
#include "../../coders/xml.hpp"
#include "../../content/Content.hpp"
#include "../../content/ContentLoader.hpp"
#include "../../engine.hpp"
#include "../../files/files.hpp"
#include "../../graphics/core/Atlas.hpp"
#include "../../graphics/core/Shader.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../graphics/render/BlocksPreview.hpp"
#include "../../graphics/ui/elements/Menu.hpp"
#include "../../graphics/ui/GUI.hpp"
#include "../../items/ItemDef.hpp"
#include "../../util/stringutil.hpp"
#include "../../window/Camera.hpp"
#include "../../window/Events.hpp"
#include "../../window/Window.hpp"
#include "../../world/Level.hpp"
#include "../ContentGfxCache.hpp"
#include "IncludeCommons.hpp"
#include "menu_workshop.hpp"
#include "WorkshopPreview.hpp"
#include "WorkshopSerializer.hpp"
#include "WorkshopUtils.hpp"

#define NOMINMAX
#include "libs/portable-file-dialogs.h"
#include "../screens/MenuScreen.hpp"

using namespace workshop;

WorkShopScreen::WorkShopScreen(Engine* engine, const ContentPack& pack) : Screen(engine),
framerate(engine->getSettings().display.framerate.get()),
gui(engine->getGUI()),
currentPack(pack),
assets(engine->getAssets())
{
	gui->getMenu()->reset();
	uicamera.reset(new Camera(glm::vec3(), static_cast<float>(Window::height)));
	uicamera->perspective = false;
	uicamera->flipped = true;

	if (framerate > 60) {
		engine->getSettings().display.framerate.set(-1);
	}
	if (initialize()) {
		createNavigationPanel();
	}
	if (fs::exists(SETTINGS_FILE)) {
		dynamic::Map_sptr settingsMap = files::read_json(SETTINGS_FILE);
		if (settingsMap->has("sensitivity")) settings.previewSensitivity = settingsMap->get<float>("sensitivity");
		dynamic::List_sptr list = settingsMap->list("customModelInputRange");
		if (list && list->size() == 2) {
			settings.customModelRange.x = list->num(0);
			settings.customModelRange.y = list->num(1);
		}
		if (settingsMap->has("contentListWidth")) settings.contentListWidth = settingsMap->get<float>("contentListWidth");
		if (settingsMap->has("blockEditorWidth")) settings.blockEditorWidth = settingsMap->get<float>("blockEditorWidth");
		if (settingsMap->has("customModelEditorWidth")) settings.customModelEditorWidth = settingsMap->get<float>("customModelEditorWidth");
		if (settingsMap->has("itemEditorWidth")) settings.itemEditorWidth = settingsMap->get<float>("itemEditorWidth");
		if (settingsMap->has("textureListWidth")) settings.textureListWidth = settingsMap->get<float>("textureListWidth");
	}
}

WorkShopScreen::~WorkShopScreen() {
	for (const auto& elem : panels) {
		gui->remove(elem.second);
	}

	dynamic::Map settingsMap;
	settingsMap.put("sensitivity", settings.previewSensitivity);
	settingsMap.put("contentListWidth", settings.contentListWidth);
	settingsMap.put("blockEditorWidth", settings.blockEditorWidth);
	settingsMap.put("customModelEditorWidth", settings.customModelEditorWidth);
	settingsMap.put("itemEditorWidth", settings.itemEditorWidth);
	settingsMap.put("textureListWidth", settings.textureListWidth);
	dynamic::List& list = settingsMap.putList("customModelInputRange");
	list.put(settings.customModelRange.x);
	list.put(settings.customModelRange.y);
	files::write_json(SETTINGS_FILE, &settingsMap);
}

void WorkShopScreen::update(float delta) {
	if (Events::jpressed(keycode::ESCAPE) && (preview && !preview->lockedKeyboardInput)) {
		if (panels.size() < 2 && !showUnsaved()) {
			exit();
		}
		else if (panels.size() > 1) removePanel(panels.rbegin()->first);
		return;
	}

	for (const auto& elem : panels) {
		elem.second->UINode::setSize(glm::vec2(elem.second->getSize().x, Window::height - 4.f));
	}
	if (preview) {
		for (const auto& elem : panels) {
			if (hasFocusedTextbox(*elem.second)) {
				preview->lockedKeyboardInput = true;
				break;
			}
		}
		preview->update(delta, settings.previewSensitivity);
		preview->lockedKeyboardInput = false;
	}
}

void WorkShopScreen::draw(float delta) {
	Window::clear();
	Window::setBgColor(glm::vec3(0.2f));

	uicamera->setFov(static_cast<float>(Window::height));
	Shader* const uishader = assets->get<Shader>("ui");
	uishader->use();
	uishader->uniformMatrix("u_projview", uicamera->getProjView());

	const uint width = Window::width;
	const uint height = Window::height;

	Texture* bg = assets->get<Texture>("gui/menubg");
	batch->begin();
	batch->texture(bg);
	batch->rect(0.f, 0.f,
		width, height, 0.f, 0.f, 0.f,
		UVRegion(0.f, 0.f, width / bg->getWidth(), height / bg->getWidth()),
		false, false, glm::vec4(1.0f));
	batch->flush();
}

bool WorkShopScreen::initialize() {
	currentPackId = currentPack.id;

	auto& packs = engine->getContentPacks();
	packs.clear();

	std::vector<ContentPack> scanned;
	ContentPack::scanFolder(engine->getPaths()->getResources() / "content", scanned);

	if (currentPackId != "base") {
		auto it = std::find_if(scanned.begin(), scanned.end(), [](const ContentPack& pack) {
			return pack.id == "base";
		});
		if (it != scanned.end()) packs.emplace_back(*it);
	}
	packs.emplace_back(currentPack);

	for (size_t i = 0; i < packs.size(); i++) {
		for (size_t j = 0; j < packs[i].dependencies.size(); j++) {
			if (std::find_if(scanned.begin(), scanned.end(), [&dependency = packs[i].dependencies[j]](const ContentPack& pack) {
				return dependency.id == pack.id;
			}) == scanned.end()) {
				createContentErrorMessage(packs[i], "Dependency \"" + packs[i].dependencies[j].id + "\" not found");
				return 0;
			}
		}
	}
	try {
		engine->loadContent();
	}
	catch (const contentpack_error& e) {
		createContentErrorMessage(*std::find_if(packs.begin(), packs.end(), [e](const ContentPack& pack) {
			return pack.id == e.getPackId();
		}), e.what());
		return 0;
	}
	catch (const std::exception& e) {
		createContentErrorMessage(packs.back(), e.what());
		return 0;
	}
	assets = engine->getAssets();
	content = engine->getContent();
	indices = content->getIndices();
	cache.reset(new ContentGfxCache(content, assets));
	previewAtlas = BlocksPreview::build(cache.get(), assets, content).release();
	assets->store(std::unique_ptr<Atlas>(previewAtlas), BLOCKS_PREVIEW_ATLAS);
	itemsAtlas = assets->get<Atlas>("items");
	blocksAtlas = assets->get<Atlas>("blocks");
	preview.reset(new Preview(engine, cache.get()));
	backupDefs();

	return 1;
}

void WorkShopScreen::exit() {
	Engine* const e = engine;
	e->getSettings().display.framerate.set(framerate);
	e->setScreen(std::make_shared<MenuScreen>(e));
	create_workshop_button(e, &e->getGUI()->getMenu()->getCurrent());
	e->getGUI()->getMenu()->setPage("workshop");
}

void WorkShopScreen::createNavigationPanel() {
	gui::Panel& panel = *new gui::Panel(glm::vec2(200.f));

	panels.emplace(0, std::shared_ptr<gui::Panel>(&panel));
	gui->add(panels.at(0));

	panel.setPos(glm::vec2(2.f));
	panel += new gui::Button(L"Info", glm::vec4(10.f), [this](gui::GUI*) {
		createPackInfoPanel();
	});
	panel += new gui::Button(L"Blocks", glm::vec4(10.f), [this](gui::GUI*) {
		createContentList(DefType::BLOCK);
	});
	panel += new gui::Button(L"Block Materials", glm::vec4(10.f), [this](gui::GUI*) {
		createMaterialsList();
	});
	panel += new gui::Button(L"Textures", glm::vec4(10.f), [this](gui::GUI*) {
		createTextureList(50.f);
	});
	panel += new gui::Button(L"Items", glm::vec4(10.f), [this](gui::GUI*) {
		createContentList(DefType::ITEM);
	});
	panel += new gui::Button(L"Sounds", glm::vec4(10.f), [this](gui::GUI*) {
		createSoundList();
	});
	panel += new gui::Button(L"Scripts", glm::vec4(10.f), [this](gui::GUI*) {
		createScriptList();
	});
	panel += new gui::Button(L"UI Layouts", glm::vec4(10.f), [this](gui::GUI*) {
		createUILayoutList();
	});
	gui::Button* button = new gui::Button(L"Back", glm::vec4(10.f), [this](gui::GUI*) {
		showUnsaved([this]() {
			exit();
		});
	});
	panel += button;

	createPackInfoPanel();

	gui::Container* container = new gui::Container(panel.getSize());
	container->listenInterval(0.01f, [&panel, button, container]() {
		panel.refresh();
		container->setSize(glm::vec2(panel.getSize().x, Window::height - (button->calcPos().y + (button->getSize().y + 7.f) * 4)));
	});
	panel += container;

	panel += new gui::Button(L"Utils", glm::vec4(10.f), [this](gui::GUI*) {
		createUtilsPanel();
	});
	panel += new gui::Button(L"Settings", glm::vec4(10.f), [this](gui::GUI*) {
		createSettingsPanel();
	});
	setSelectable<gui::Button>(panel);

	panel += new gui::Button(L"Open Pack Folder", glm::vec4(10.f), [this](gui::GUI*) {
		openPath(currentPack.folder);
	});
}

void WorkShopScreen::createContentErrorMessage(ContentPack& pack, const std::string& message) {
	auto panel = std::make_shared<gui::Panel>(glm::vec2(500.f, 20.f));
	panel->listenInterval(0.1f, [panel]() {
		panel->setPos(Window::size() / 2.f - panel->getSize() / 2.f);
	});

	*panel += new gui::Label("Error in content pack \"" + pack.id + "\":");
	gui::Label* label = new gui::Label(message);
	label->setMultiline(true);
	*panel += label;

	*panel += new gui::Button(L"Open pack folder", glm::vec4(10.f), [pack](gui::GUI*) {
		openPath(pack.folder);
	});
	*panel += new gui::Button(L"Back", glm::vec4(10.f), [this, panel](gui::GUI*) { gui->remove(panel); exit(); });

	panel->setPos(Window::size() / 2.f - panel->getSize() / 2.f);
	gui->add(panel);
}

void WorkShopScreen::removePanel(unsigned int column) {
	gui->remove(panels[column]);
	panels.erase(column);
}

void WorkShopScreen::removePanels(unsigned int column) {
	for (auto it = panels.begin(); it != panels.end();) {
		if (it->first >= column) {
			gui->remove(it->second);
			panels.erase(it++);
		}
		else ++it;
	}
}

void WorkShopScreen::clearRemoveList(gui::Panel& panel) {
	removeList.erase(std::remove_if(removeList.begin(), removeList.end(), [&panel](gui::UINode* node) {
		for (const auto& elem : panel.getNodes()) {
			if (elem.get() == node) {
				panel.remove(elem);
				return true;
			}
		}
		return false;
	}), removeList.end());
}

void WorkShopScreen::createPanel(const std::function<gui::Panel& ()>& lambda, unsigned int column, float posX) {
	removePanels(column);

	auto panel = std::shared_ptr<gui::Panel>(&lambda());
	panels.emplace(column, panel);

	if (posX == PANEL_POSITION_AUTO) {
		float x = 2.f;
		for (const auto& elem : panels) {
			elem.second->setPos(glm::vec2(x, 2.f));
			elem.second->act(0.1f);
			x += elem.second->getSize().x;
		}
	}
	else {
		panel->setPos(glm::vec2(posX, 2.f));
	}
	gui->add(panel);
}

void WorkShopScreen::createTexturesPanel(gui::Panel& panel, float iconSize, std::string* textures, BlockModel model) {
	panel.setColor(glm::vec4(0.f));

	const char* const faces[] = { "East:", "West:", "Bottom:", "Top:", "South:", "North:" };

	size_t buttonsNum = 0;

	switch (model) {
		case BlockModel::block:
		case BlockModel::aabb: buttonsNum = 6;
			break;
		case BlockModel::xsprite: buttonsNum = 5;
			break;
		case BlockModel::custom: buttonsNum = 1;
			break;
		default: break;
	}
	if (buttonsNum == 0) return;
	panel += removeList.emplace_back(new gui::Label(buttonsNum == 1 ? L"Texture" : L"Texture faces"));
	for (size_t i = 0; i < buttonsNum; i++) {
		gui::IconButton* button = new gui::IconButton(glm::vec2(panel.getSize().x, iconSize), textures[i], blocksAtlas, textures[i],
			(buttonsNum == 6 ? faces[i] : ""));
		button->listenAction([this, button, model, textures, iconSize, i](gui::GUI*) {
			createTextureList(35.f, 5, DefType::BLOCK, button->calcPos().x + button->getSize().x, true,
			[this, button, model, textures, iconSize, i](const std::string& texName) {
				textures[i] = getTexName(texName);
				removePanel(5);
				button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
				button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
				button->setIcon(getAtlas(assets, texName), getTexName(texName));
				button->setText(getTexName(texName));
				preview->updateCache();
			});
		});
		panel += removeList.emplace_back(button);
		if (model == BlockModel::xsprite && i == 0) i = 3;
	}

	setSelectable<gui::IconButton>(panel);
}

void WorkShopScreen::createTexturesPanel(gui::Panel& panel, float iconSize, std::string& texture, item_icon_type iconType) {
	panel.setColor(glm::vec4(0.f));
	if (iconType == item_icon_type::none) return;

	auto texName = [this, iconType, &texture]() {
		if (iconType == item_icon_type::sprite) return getTexName(texture);
		return texture;
	};

	gui::IconButton* button = new gui::IconButton(glm::vec2(panel.getSize().x, iconSize), texName(), getAtlas(assets, texture), texName());
	button->listenAction([this, button, texName, &panel, &texture, iconSize, iconType](gui::GUI*) {
		auto& nodes = button->getNodes();
		auto callback = [this, button, nodes, texName, iconSize, &texture](const std::string& textureName) {
			texture = textureName;
			removePanel(5);
			button->setIcon(getAtlas(assets, texture), texName());
			button->setText(texName());
		};
		if (iconType == item_icon_type::sprite) {
			createTextureList(35.f, 5, DefType::BOTH, PANEL_POSITION_AUTO, true, callback);
		}
		else {
			createContentList(DefType::BLOCK, 5, true, callback);
		}
		button->setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.95f));
		button->setHoverColor(glm::vec4(0.05f, 0.1f, 0.15f, 0.75f));
	});
	panel += removeList.emplace_back(button);
}

void WorkShopScreen::createAddingUIElementPanel(float posX, const std::function<void(const std::string&)>& callback, unsigned int filter) {
	createPanel([this, callback, filter]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200.f));

		for (const auto& elem : uiElementsArgs) {
			if (elem.second.args & UIElementsArgs::INVENTORY || filter & elem.second.args) continue;
			const std::string& name(elem.first);
			panel += new gui::Button(util::str2wstr_utf8(name), glm::vec4(10.f), [this, callback, &name](gui::GUI*) {
				callback(name);
			});
		}
		return std::ref(panel);
	}, 5, posX);
}

void WorkShopScreen::createMaterialsList(bool showAll, unsigned int column, float posX, const std::function<void(const std::string&)>& callback) {
	createPanel([this, showAll, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		for (auto& elem : content->getBlockMaterials()) {
			if (!showAll && elem.first.substr(0, currentPackId.length()) != currentPackId) continue;
			gui::Button& button = *new gui::Button(util::str2wstr_utf8(elem.first), glm::vec4(10.f), [this, &elem, callback](gui::GUI*) {
				if (callback) callback(elem.first);
				else createMaterialEditor(const_cast<BlockMaterial&>(*elem.second));
			});
			button.setTextAlign(gui::Align::left);
			panel += button;
		}

		return std::ref(panel);
	}, column, posX);
}

void WorkShopScreen::createScriptList(unsigned int column, float posX, const std::function<void(const std::string&)>& callback) {
	createPanel([this, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		fs::path scriptsFolder(currentPack.folder / "scripts/");
		std::vector<fs::path> scripts = getFiles(scriptsFolder, true);
		if (callback) scripts.emplace(scripts.begin(), fs::path());

		for (const auto& elem : scripts) {
			fs::path parentDir(fs::relative(elem.parent_path(), scriptsFolder));
			std::string fullScriptName = parentDir.string() + "/" + elem.stem().string();
			std::string scriptName = fs::path(getScriptName(currentPack, fullScriptName)).stem().string();
			gui::Button& button = *new gui::Button(util::str2wstr_utf8(scriptName), glm::vec4(10.f),
				[this, elem, fullScriptName, callback](gui::GUI*) {
				if (callback) callback(fullScriptName);
				else createScriptInfoPanel(elem);
			});
			button.setTextAlign(gui::Align::left);
			panel += button;
		}

		setSelectable<gui::Button>(panel);

		return std::ref(panel);
	}, column, posX);
}

void WorkShopScreen::createScriptInfoPanel(const fs::path& file) {
	createPanel([this, file]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(400));

		std::string fileName(file.stem().string());
		std::string extention(file.extension().string());

		panel += new gui::Label("Path: " + fs::relative(file, currentPack.folder).remove_filename().string());
		panel += new gui::Label("File: " + fileName + extention);

		panel += new gui::Button(L"Open script", glm::vec4(10.f), [file](gui::GUI*) {
			openPath(file);
		});

		return std::ref(panel);
	}, 2);
}

void WorkShopScreen::createSoundList() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		std::vector<fs::path> sounds = getFiles(currentPack.folder / SOUNDS_FOLDER, true);

		for (const auto& elem : sounds) {
			gui::Button& button = *new gui::Button(util::str2wstr_utf8(elem.stem().string()), glm::vec4(10.f), [this, elem](gui::GUI*) {
				createSoundInfoPanel(elem);
			});
			button.setTextAlign(gui::Align::left);
			panel += button;
		}

		return std::ref(panel);
	}, 1);
}

void WorkShopScreen::createSoundInfoPanel(const fs::path& file) {
	createPanel([this, file]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(400));

		std::string fileName(file.stem().string());
		std::string extention(file.extension().string());

		panel += new gui::Label("Path: " + fs::relative(file, currentPack.folder).remove_filename().string());
		panel += new gui::Label("File: " + fileName + extention);

		fs::path parentDir = fs::relative(file.parent_path(), currentPack.folder / SOUNDS_FOLDER);

		//audio::Sound* sound = assets->get<audio::Sound>(parentDir.empty() ? "" : parentDir.string() + "/" + fileName);
		//sound->newInstance(0, 0);

		return std::ref(panel);
	}, 2);
}

void workshop::WorkShopScreen::createSettingsPanel() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));

		panel += new gui::Label(L"Custom model input range");
		panel += createNumTextBox(settings.customModelRange.x, L"Min", -FLT_MAX, FLT_MAX);
		panel += createNumTextBox(settings.customModelRange.y, L"Max", -FLT_MAX, FLT_MAX);

		panel += new gui::Label(L"Content list width");
		panel += createNumTextBox(settings.contentListWidth, L"", 100.f, 1000.f);
		panel += new gui::Label(L"Block editor width");
		panel += createNumTextBox(settings.blockEditorWidth, L"", 100.f, 1000.f);
		panel += new gui::Label(L"Custom model editor width");
		panel += createNumTextBox(settings.customModelEditorWidth, L"", 100.f, 1000.f);
		panel += new gui::Label(L"Item editor width");
		panel += createNumTextBox(settings.itemEditorWidth, L"", 100.f, 1000.f);
		panel += new gui::Label(L"Texture list width");
		panel += createNumTextBox(settings.textureListWidth, L"", 100.f, 1000.f);

		gui::Label& label = *new gui::Label(L"Preview sensitivity: " + util::to_wstring(settings.previewSensitivity, 2));
		panel += label;
		gui::TrackBar& trackBar = *new gui::TrackBar(0.1, 5.0, settings.previewSensitivity, 0.01);
		trackBar.setConsumer([this, &label](double num) {
			settings.previewSensitivity = num;
			label.setText(L"Preview window sensitivity: " + util::to_wstring(settings.previewSensitivity, 2));
		});
		panel += trackBar;

		return std::ref(panel);
	}, 1);
}

void workshop::WorkShopScreen::createUtilsPanel() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));

		panel += new gui::Button(L"Fix aabb's", glm::vec4(10.f), [this](gui::GUI*) {
			std::unordered_map<Block*, size_t> brokenAABBs;
			for (size_t i = 0; i < indices->blocks.count(); i++) {
				size_t aabbsNum = 0;
				Block* block = indices->blocks.get(i);
				for (auto& aabb : block->modelBoxes) {
					if (aabb.a != aabb.min() && aabb.b != aabb.max()) aabbsNum++;
				}
				if (aabbsNum > 0) brokenAABBs.emplace(block, aabbsNum);
			}
			createPanel([this, brokenAABBs]() {
				gui::Panel& panel = *new gui::Panel(glm::vec2(300));

				if (brokenAABBs.empty()) {
					panel += new gui::Label(L"Wrong aabb's not found");
				}
				else {
					panel += new gui::Button(L"Fix all", glm::vec4(10.f), [this, brokenAABBs](gui::GUI*) {
						for (const auto& pair : brokenAABBs) {
							for (auto& aabb : pair.first->modelBoxes) {
								AABB temp(aabb.min(), aabb.max());
								aabb = temp;
							}
						}
						removePanel(2);
					});
					for (const auto& pair : brokenAABBs) {
						panel += new gui::Label("Block: " + getDefName(pair.first->name));
						panel += new gui::Label("Wrong aabb's: " + std::to_string(pair.second));
					}
				}

				return std::ref(panel);
			}, 2);
		});

		panel += new gui::Button(L"Find unused textures", glm::vec4(10.f), [this](gui::GUI*) {
			fs::path blocksTexturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::BLOCKS_FOLDER);
			fs::path itemsTexturesPath(currentPack.folder / TEXTURES_FOLDER / ContentPack::ITEMS_FOLDER);

			std::unordered_multimap<DefType, fs::path> unusedTextures;

			for (const auto& path : getFiles(blocksTexturesPath, false)) {
				const std::string fileName = path.stem().string();
				bool inUse = false;
				for (size_t i = 0; i < indices->blocks.count() && !inUse; i++) {
					const Block* const block = indices->blocks.get(i);
					if (std::find(std::begin(block->textureFaces), std::end(block->textureFaces), fileName) != std::end(block->textureFaces)) inUse = true;
					if (std::find(block->modelTextures.begin(), block->modelTextures.end(), fileName) != block->modelTextures.end()) inUse = true;
				}
				for (size_t i = 0; i < indices->items.count() && !inUse; i++) {
					const ItemDef* const item = indices->items.get(i);
					if (item->iconType == item_icon_type::sprite && getTexName(item->icon) == fileName) inUse = true;
				}
				if (!inUse) unusedTextures.emplace(DefType::BLOCK, path);
			}
			for (const auto& path : getFiles(itemsTexturesPath, false)) {
				const std::string fileName = path.stem().string();
				bool inUse = false;
				for (size_t i = 0; i < indices->items.count() && !inUse; i++) {
					const ItemDef* const item = indices->items.get(i);
					if (item->iconType == item_icon_type::sprite && getTexName(item->icon) == fileName) inUse = true;
				}
				if (!inUse) unusedTextures.emplace(DefType::ITEM, path);
			}
			createPanel([this, unusedTextures]() {
				gui::Panel& panel = *new gui::Panel(glm::vec2(300));

				if (unusedTextures.empty()) {
					panel += new gui::Label("Unused textures not found");
				}
				else {
					panel += new gui::Label("Found " + std::to_string(unusedTextures.size()) + " unused textures");
					panel += new gui::Button(L"Delete all", glm::vec4(10.f), [this, unusedTextures](gui::GUI*){
						std::vector<fs::path> v;
						std::transform(unusedTextures.begin(), unusedTextures.end(), std::back_inserter(v), [](const auto& pair) {return pair.second; });
						createFileDeletingConfirmationPanel(v, 3, [this]() {
							initialize();
							createUtilsPanel();
						});
					});
					for (const auto& pair : unusedTextures) {
						const Atlas* atlas = assets->get<Atlas>(getDefFolder(pair.first));
						const std::string file = pair.second.stem().string();

						gui::IconButton& button = *new gui::IconButton(glm::vec2(panel.getSize().x, 50.f), file, atlas, file);
						button.listenAction([this, file, pair](gui::GUI*) {
							createTextureInfoPanel(getDefFolder(pair.first) + ':' + file, pair.first, 3);
						});
						panel += button;
					}
				}

				return std::ref(panel);
			}, 2);
		});

		return std::ref(panel);
	}, 1);
}

void WorkShopScreen::createTextureInfoPanel(const std::string& texName, DefType type, unsigned int column) {
	createPanel([this, texName, type]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(350));

		panel += new gui::Label(texName);
		gui::Container& imageContainer = *new gui::Container(glm::vec2(panel.getSize().x));
		const Atlas* const atlas = getAtlas(assets, texName);
		Texture* const tex = atlas->getTexture();
		gui::Image& image = *new gui::Image(tex, glm::vec2(0.f));
		formatTextureImage(image, atlas, panel.getSize().x, getTexName(texName));
		imageContainer += image;
		panel += imageContainer;
		const UVRegion& uv = atlas->get(getTexName(texName));
		const glm::ivec2 size((uv.u2 - uv.u1) * tex->getWidth(), (uv.v2 - uv.v1) * tex->getHeight());
		panel += new gui::Label(L"Width: " + std::to_wstring(size.x));
		panel += new gui::Label(L"Height: " + std::to_wstring(size.y));
		panel += new gui::Label(L"Texture type: " + util::str2wstr_utf8(getDefName(type)));
		panel += new gui::Button(L"Delete", glm::vec4(10.f), [this, texName, type](gui::GUI*) {
			showUnsaved([this, texName, type]() {
				createFileDeletingConfirmationPanel({ currentPack.folder / TEXTURES_FOLDER / getDefFolder(type) / std::string(getTexName(texName) + ".png") },
				3, [this]() {
					initialize();
					createTextureList(50.f, 1);
				});
			});
		});
		return std::ref(panel);
	}, column);
}

void WorkShopScreen::createImportPanel(DefType type, std::string mode) {
	createPanel([this, type, mode]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		panel += new gui::Label(L"Import texture");
		panel += new gui::Button(L"Import as: " + util::str2wstr_utf8(getDefName(type)), glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			switch (type) {
				case DefType::BLOCK: createImportPanel(DefType::ITEM, mode); break;
				case DefType::ITEM: createImportPanel(DefType::BLOCK, mode); break;
			}
		});
		panel += new gui::Button(L"Import mode: " + util::str2wstr_utf8(mode), glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			if (mode == "copy") createImportPanel(type, "move");
			else if (mode == "move") createImportPanel(type, "copy");
		});
		panel += new gui::Button(L"Select files", glm::vec4(10.f), [this, type, mode](gui::GUI*) {
			showUnsaved([this, type, mode]() {
				auto files = pfd::open_file("", "", { "(.png)", "*.png" }, pfd::opt::multiselect).result();
				if (files.empty()) return;
				fs::path path(currentPack.folder / TEXTURES_FOLDER / getDefFolder(type));
				if (!fs::is_directory(path)) fs::create_directories(path);
				for (const auto& elem : files) {
					if (fs::is_regular_file(path / fs::path(elem).filename())) continue;
					fs::copy(elem, path);
					if (mode == "move") fs::remove(elem);
				}
				initialize();
				createTextureList(50.f);
			});
		});

		return std::ref(panel);
	}, 2);
}

void workshop::WorkShopScreen::createFileDeletingConfirmationPanel(const std::vector<fs::path>& files, unsigned int column,
	const std::function<void(void)>& callback)
{
	createPanel([this, files, column, callback]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));

		gui::Label& label = *new gui::Label("Are you sure you want to permanently delete this file(s)?");
		label.setTextWrapping(true);
		label.setMultiline(true);
		label.setSize(panel.getSize() / 4.f);
		panel += label;

		gui::Panel& filesList = *new gui::Panel(panel.getSize());
		filesList.setMaxLength(300);
		for (const auto& file : files){
			filesList += new gui::Label(fs::relative(file, currentPack.folder).string());
		}
		panel += filesList;

		panel += new gui::Button(L"Confirm", glm::vec4(10.f), [this, files, column, callback](gui::GUI*) {
			if (showUnsaved([this, files, column, callback]() {
				for (const auto& file : files){
					fs::remove(file);
				}
				removePanel(column);
				if (callback) callback();
			}));
		});
		panel += new gui::Button(L"Cancel", glm::vec4(10.f), [this, column](gui::GUI*) {
			removePanel(column);
		});

		return std::ref(panel);
	}, column);
}

void WorkShopScreen::createMaterialEditor(BlockMaterial& material) {
	createPanel([this, &material]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(200));

		panel += new gui::Label(material.name);

		panel += new gui::Label("Step Sound");
		createTextBox(panel, material.stepsSound);
		panel += new gui::Label("Place Sound");
		createTextBox(panel, material.placeSound);
		panel += new gui::Label("Break Sound");
		createTextBox(panel, material.breakSound);

		return std::ref(panel);
	}, 2);
}

void WorkShopScreen::createBlockPreview(unsigned int column, PrimitiveType type) {
	createPanel([this, type]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));
		gui::Image& image = *new gui::Image(preview->getTexture(), glm::vec2(panel.getSize().x));
		panel += image;
		panel.listenInterval(0.01f, [this, &panel, &image]() {
			if (panel.isHover() && image.isInside(Events::cursor)) {
				if (Events::jclicked(mousecode::BUTTON_1)) preview->lmb = true;
				if (Events::jclicked(mousecode::BUTTON_2)) preview->rmb = true;
				if (Events::scroll) preview->scale(-Events::scroll / 10.f);
			}
			panel.setSize(glm::vec2(Window::width - panel.calcPos().x - 2.f, Window::height));
			image.setSize(glm::vec2(panel.getSize().x, Window::height / 2.f));
			preview->setResolution(static_cast<uint>(image.getSize().x), static_cast<uint>(image.getSize().y));
			preview->drawBlock();
			image.setTexture(preview->getTexture());
		});
		createFullCheckBox(panel, L"Draw grid", preview->drawGrid);
		createFullCheckBox(panel, L"Show front direction", preview->drawDirection);
		createFullCheckBox(panel, L"Draw block size", preview->drawBlockSize);
		if (type == PrimitiveType::HITBOX)
			createFullCheckBox(panel, L"Draw current hitbox", preview->drawBlockHitbox);
		else if (type == PrimitiveType::AABB)
			createFullCheckBox(panel, L"Highlight current AABB", preview->drawCurrentAABB);
		else if (type == PrimitiveType::TETRAGON)
			createFullCheckBox(panel, L"Highlight current Tetragon", preview->drawCurrentTetragon);
		panel += new gui::Label("");
		panel += new gui::Label("Camera control");
		panel += new gui::Label("Position: RMB + mouse");
		panel += new gui::Label("Rotate: W,S,A,D or LMB + mouse");
		panel += new gui::Label("Zoom: Left Shift/Space or Scroll Wheel");

		return std::ref(panel);
	}, column);
}

void WorkShopScreen::createUIPreview() {
	createPanel([this]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(300));
		gui::Image& image = *new gui::Image(preview->getTexture(), glm::vec2(panel.getSize().x, Window::height));
		panel += image;
		panel.listenInterval(0.01f, [this, &panel, &image]() {
			panel.setSize(glm::vec2(Window::width - panel.calcPos().x - 2.f, Window::height));
			image.setSize(glm::vec2(image.getSize().x, Window::height));
			preview->setResolution(Window::width, Window::height);
			image.setTexture(preview->getTexture());
			image.setUVRegion(UVRegion(0.f, 0.f, image.getSize().x / Window::width, 1.f));
			preview->drawUI();
		});

		return std::ref(panel);
	}, 3);
}

void workshop::WorkShopScreen::backupDefs() {
	blocksList.clear();
	itemsList.clear();
	for (size_t i = 0; i < indices->blocks.count(); i++) {
		const Block* const block = indices->blocks.get(i);
		if (block->name.find(currentPackId) == std::string::npos) continue;
		std::string blockName(getDefName(block->name));
		blocksList[blockName] = stringify(*block, blockName, false);
	}
	for (size_t i = 0; i < indices->items.count(); i++) {
		const ItemDef* const item = indices->items.get(i);
		if (item->name.find(currentPackId) == std::string::npos) continue;
		std::string itemName(getDefName(item->name));
		itemsList[itemName] = stringify(*item, itemName, false);
	}
}

bool workshop::WorkShopScreen::showUnsaved(const std::function<void()>& callback) {
	std::unordered_multimap<DefType, std::string> unsavedList;

	for (const auto& elem : blocksList) {
		if (elem.second != stringify(*content->blocks.find(currentPackId + ':' + elem.first), elem.first, false))
			unsavedList.emplace(DefType::BLOCK, elem.first);
	}
	for (const auto& elem : itemsList) {
		if (elem.second != stringify(*content->items.find(currentPackId + ':' + elem.first), elem.first, false))
			unsavedList.emplace(DefType::ITEM, elem.first);
	}

	if (unsavedList.empty() || ignoreUnsaved) {
		ignoreUnsaved = false;
		if (callback) callback();
		return false;
	}
	gui::Container& container = *new gui::Container(Window::size());
	std::shared_ptr<gui::Container> containerPtr(&container);
	container.setColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));

	gui::Panel& outerPanel = *new gui::Panel(Window::size() / 2.f);
	outerPanel.setColor(glm::vec4(0.5f, 0.5f, 0.5f, 0.25f));
	outerPanel.listenInterval(0.01f, [this, &outerPanel, &container]() {
		preview->lockedKeyboardInput = true;
		outerPanel.setPos(Window::size() / 2.f - outerPanel.getSize() / 2.f);
		container.setSize(Window::size());
	});
	outerPanel += new gui::Label("You have " + std::to_string(unsavedList.size()) + " unsaved change(s):");

	gui::Panel& innerPanel = *new gui::Panel(glm::vec2());
	innerPanel.setColor(glm::vec4(0.f));
	innerPanel.setMaxLength(Window::height / 3.f);

	for (const auto& elem : unsavedList) {
		innerPanel += new gui::Label(getDefName(elem.first) + ": " + elem.second);
	}

	outerPanel += innerPanel;
	gui::Container& buttonsContainer = *new gui::Container(glm::vec2(outerPanel.getSize().x, 20));
	buttonsContainer += new gui::Button(L"Back", glm::vec4(10.f), [this, containerPtr](gui::GUI*) { gui->remove(containerPtr); });
	buttonsContainer += new gui::Button(L"Save all", glm::vec4(10.f), [this, containerPtr, unsavedList, callback](gui::GUI*) {
		for (const auto& pair : unsavedList) {
			if (pair.first == DefType::BLOCK)
				saveBlock(*content->blocks.find(currentPackId + ':' + pair.second), currentPack.folder, pair.second);
			else if (pair.first == DefType::ITEM)
				saveItem(*content->items.find(currentPackId + ':' + pair.second), currentPack.folder, pair.second);
		}
		backupDefs();
		gui->remove(containerPtr);
		if (callback) callback();
	});
	buttonsContainer += new gui::Button(L"Discard all", glm::vec4(10.f), [this, containerPtr](gui::GUI*) {
		initialize();
		removePanels(0);
		createNavigationPanel();
		gui->remove(containerPtr);
	});
	buttonsContainer += new gui::Button(L"Ignore once", glm::vec4(10.f), [this, containerPtr, callback](gui::GUI*) {
		ignoreUnsaved = true;
		gui->remove(containerPtr);
		if (callback) callback();
	});
	placeNodesHorizontally(buttonsContainer);
	outerPanel += buttonsContainer;
	outerPanel.setPos(Window::size() / 2.f - outerPanel.getSize() / 2.f);

	container += outerPanel;
	gui->add(containerPtr);

	return true;
}