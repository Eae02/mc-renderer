#pragma once

#include <glm/glm.hpp>
#include <SDL2/SDL_events.h>
#include "devmenu.h"

namespace MCR
{
	class DevMenuBar
	{
	public:
		DevMenuBar() = default;
		
		void Render(class UIDrawList& drawList, glm::ivec2 screenSize) const;
		
		void KeyPressEvent(const SDL_KeyboardEvent& keyEvent);
		
		inline void AddMenu(std::string_view name, std::unique_ptr<DevMenu> menu)
		{
			m_menus.push_back(std::make_pair(name, std::move(menu)));
		}
		
	private:
		bool m_hasFocus = false;
		bool m_subMenuHasFocus = false;
		
		std::vector<std::pair<std::string_view, std::unique_ptr<DevMenu>>> m_menus;
		long m_currentMenu = 0;
	};
}
