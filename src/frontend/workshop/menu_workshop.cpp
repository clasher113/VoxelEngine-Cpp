#include "menu_workshop.hpp"

#include "../../coders/png.hpp"
#include "../../engine.hpp"
#include "../../graphics/core/Texture.hpp"
#include "../../graphics/ui/elements/Menu.hpp"
#include "../../graphics/ui/GUI.hpp"
#include "../../graphics/ui/gui_util.hpp"
#include "../../util/stringutil.hpp"
#include "IncludeCommons.hpp"
#include "WorkshopScreen.hpp"
#include "WorkshopSerializer.hpp"
#include "../../constants.hpp"
#include "../../graphics/ui/elements/Container.hpp"

void workshop::create_workshop_button(Engine* engine, gui::Page* page) {
    if (page->name != "main") return;

	auto menu = engine->getGUI()->getMenu();
	auto button = std::make_shared<gui::Button>(L"Workshop", glm::vec4(10.f), [menu](gui::GUI*) {
		menu->setPage("workshop");
	});

	auto panel = std::dynamic_pointer_cast<gui::Panel>(page->panel);
	if (!panel) return;
	std::vector<std::shared_ptr<gui::UINode>> nodes = panel->getNodes();
	panel->clear();
	for (size_t i = 0; i < nodes.size(); i++) {
		if (i == nodes.size() - 1) panel->add(button);
		panel->add(nodes[i]);
	}
	menu->setSize(panel->getSize());

	panel = std::make_shared<gui::Panel>(glm::vec2(400));
	menu->addPage("workshop", panel);
	auto packsPanel = std::make_shared<gui::Panel>(panel->getSize());
	packsPanel->setColor(glm::vec4(0.f));
	packsPanel->setMaxLength(500);
	panel->add(packsPanel);

	std::vector<ContentPack> scanned;
	ContentPack::scanFolder(engine->getPaths()->getResources()/"content", scanned);

	for (const auto& pack : scanned) {
		auto container = std::make_shared<gui::Container>(glm::vec2(panel->getSize().x, 80.f));
		container->setColor(glm::vec4(0.f, 0.f, 0.f, 0.25f));
		container->setHoverColor(glm::vec4(0.f, 0.f, 0.f, 0.5f));
		container->listenAction([engine, pack](gui::GUI*) {
			engine->setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
		});

		auto getIcon = [engine, pack]()->std::string {
			std::string icon = pack.id + ".icon";
			Assets* assets = engine->getAssets();
			Texture* iconTex = assets->get<Texture>(icon);
			if (iconTex == nullptr) {
				auto iconfile = pack.folder / fs::path("icon.png");
				if (fs::is_regular_file(iconfile)) {
					assets->store(png::load_texture(iconfile.string()), icon);
				}
				else return "gui/no_icon";
			}
			return icon;
		};

		container->add(std::make_shared<gui::Image>(getIcon(), glm::vec2(64.f)), glm::vec2(8.f));
		container->add(std::make_shared<gui::Label>(pack.title), glm::vec2(78.f, 6.f));
		auto label = std::make_shared<gui::Label>(pack.description);
		label->setColor(glm::vec4(1.f, 1.f, 1.f, 0.7f));
		container->add(label, glm::vec2(80.f, 28.f));

		packsPanel->add(container);
	}

	panel->add(std::make_shared<gui::Button>(L"Create new", glm::vec4(10.f), [engine, menu, scanned](gui::GUI*) {
		auto panel = std::make_shared<gui::Panel>(glm::vec2(400, 200));
		menu->addPage("new-content", panel);
		menu->setPage("new-content");
		fs::path path = engine->getPaths()->getResources() / "content";

		panel->add(std::make_shared<gui::Label>(L"Name"));
		auto nameInput = std::make_shared<gui::TextBox>(L"example_pack", glm::vec4(6.0f));
		nameInput->setTextValidator([scanned](const std::wstring& text) {
			return checkPackId(text, scanned);
		});
		panel->add(nameInput);

		panel->add(std::make_shared<gui::Button>(L"Create", glm::vec4(10.f), [=](gui::GUI*) {
			if (!nameInput->validate()) return;
			ContentPack pack;
			pack.folder = path / nameInput->getInput();
			pack.id = pack.title = util::wstr2str_utf8(nameInput->getInput());
			pack.version = "1.0";
			fs::create_directories(pack.folder / ContentPack::BLOCKS_FOLDER);
			fs::create_directories(pack.folder / ContentPack::ITEMS_FOLDER);
            fs::create_directories(pack.folder / TEXTURES_FOLDER);
			saveContentPack(pack);
			menu->remove(panel);

			engine->setScreen(std::make_shared<workshop::WorkShopScreen>(engine, pack));
		}));
		panel->add(std::make_shared<gui::Button>(L"Back", glm::vec4(10.f), [menu](gui::GUI*) {
			menu->back();
		}));
	}));
	panel->add(std::make_shared<gui::Button>(L"Back", glm::vec4(10.f), [menu](gui::GUI*) {
		menu->back();
	}));
}