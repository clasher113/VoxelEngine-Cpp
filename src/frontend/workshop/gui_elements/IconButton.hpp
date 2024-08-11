#ifndef FRONTEND_MENU_WORKSHOP_GUI_ICON_BUTTON_HPP
#define FRONTEND_MENU_WORKSHOP_GUI_ICON_BUTTON_HPP

#include "../../../graphics/ui/elements/Container.hpp"

class Atlas;
class Texture;

namespace gui {
	class Image;
	class Label;

	class IconButton : public Container {
	public:
		IconButton(glm::vec2 size, const std::string& text, Texture* texture, const std::string& additionalText = "");
		IconButton(glm::vec2 size, const std::string& text, Atlas* atlas, const std::string& textureName, const std::string& additionalText = "");
		~IconButton();

		void setIcon(Atlas* atlas, const std::string& textureName);
		void setIcon(Texture* texture);
		void setText(const std::string& text);

	private:

		std::shared_ptr<Label> label;
		std::shared_ptr<Image> image;
	};
}

#endif // !FRONTEND_MENU_WORKSHOP_GUI_ICON_BUTTON_HPP