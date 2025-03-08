#ifndef FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP
#define FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP

#include <functional>
#include <glm/fwd.hpp>
#include <string>

namespace gui {
	class TextBox;
	class Container;
	class FullCheckBox;
}

template<glm::length_t L, typename T>
using vec_t = glm::vec<L, T, glm::defaultp>;

namespace workshop {
	extern gui::TextBox& createTextBox(gui::Container& container, std::string& string, const std::wstring& placeholder = L"Type here");
	extern gui::FullCheckBox& createFullCheckBox(gui::Container& container, const std::wstring& string, bool& isChecked, const std::wstring& tooltip = L"");
	template<glm::length_t L, typename T>
	extern gui::Container& createVectorPanel(vec_t<L, T>& vec, vec_t<L, T> min, vec_t<L, T> max, float width, unsigned int inputType, const std::function<void()>& callback);
	extern void createEmissionPanel(gui::Container& container, uint8_t* emission);
	template<typename T>
	extern gui::TextBox& createNumTextBox(T& value, const std::wstring& placeholder, T min, T max, const std::function<void(T)>& callback = [](T) {});
}

#endif // !FRONTEND_MENU_WORKSHOP_GUI_BASIC_ELEMENTS_HPP
