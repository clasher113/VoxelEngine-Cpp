#ifndef FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP
#define FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP

#include <functional>
#include <glm/fwd.hpp>
#include <memory>
#include <string>

namespace gui {
	class TextBox;
	class Container;
	class FullCheckBox;
	class UINode;
	class Panel;
}

template<typename T>
using vec_t = glm::vec<3, T, glm::defaultp>;

namespace workshop {
	extern std::shared_ptr<gui::TextBox> createTextBox(std::shared_ptr<gui::Container> container, std::string& string,
		const std::wstring& placeholder = L"Type here");
	extern std::shared_ptr<gui::FullCheckBox> createFullCheckBox(std::shared_ptr<gui::Container> container, const std::wstring& string, bool& isChecked, const std::wstring& tooltip = L"");
	template<typename T>
	extern std::shared_ptr<gui::UINode> createVectorPanel(vec_t<T>& vec, vec_t<T> min, vec_t<T> max, float width, unsigned int inputType, const std::function<void()>& callback);
	extern void createEmissionPanel(std::shared_ptr<gui::Container> container, uint8_t* emission);
	template<typename T>
	extern std::shared_ptr<gui::TextBox> createNumTextBox(T& value, const std::wstring& placeholder, T min, T max,
		const std::function<void(T)>& callback = [](T) {});
}

#endif // !FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP