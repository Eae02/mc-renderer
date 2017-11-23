#include "devmenubar.h"
#include "uidrawlist.h"
#include "font.h"

namespace MCR
{
	void DevMenuBar::Render(UIDrawList& drawList, glm::ivec2 screenSize) const
	{
		const Font& font = Font::GetStandardDev();
		
		const float padding = 4;
		const float height = font.GetSize() + padding;
		
		drawList.AddRectangle(glm::vec2(0, 0), glm::vec2(screenSize.x, height),
		                      glm::vec4(DevMenu::BackgroundColor, 0.95f));
		
		float x = 0;
		for (size_t i = 0; i < m_menus.size(); i++)
		{
			const glm::vec2 textSize = font.GetTextExtents(m_menus[i].first);
			float endX = x + textSize.x + padding * 2;
			
			if (m_hasFocus && m_currentMenu == static_cast<long>(i))
			{
				if (!m_subMenuHasFocus)
				{
					drawList.AddRectangle(glm::vec2(x, 0), glm::vec2(endX, height), glm::vec4(1, 1, 1, 0.2f));
				}
				
				m_menus[i].second->Render(drawList, glm::vec2(x, height));
			}
			
			drawList.AddText(font, m_menus[i].first, glm::vec2(x + padding, height / 2.0f),
			                 glm::vec4(DevMenu::TextColor, 1.0f), TextPosX::Right, TextPosY::Center);
			
			x = endX;
		}
	}
	
	void DevMenuBar::KeyPressEvent(const SDL_KeyboardEvent& keyEvent)
	{
		const SDL_Scancode activateKey = SDL_SCANCODE_TAB;
		const SDL_Scancode deactivateKey = SDL_SCANCODE_ESCAPE;
		
		if (!m_hasFocus)
		{
			if (keyEvent.keysym.scancode == activateKey)
			{
				m_hasFocus = true;
				m_subMenuHasFocus = false;
			}
			else
			{
				return;
			}
		}
		
		if (keyEvent.keysym.scancode == deactivateKey)
		{
			if (m_subMenuHasFocus)
			{
				m_subMenuHasFocus = false;
				m_menus[m_currentMenu].second->Deactivate();
			}
			else
			{
				m_hasFocus = false;
				return;
			}
		}
		
		if (m_subMenuHasFocus)
		{
			if (m_menus[m_currentMenu].second->GetSelectedEntry() == 0 && keyEvent.keysym.scancode == SDL_SCANCODE_UP)
			{
				m_subMenuHasFocus = false;
				m_menus[m_currentMenu].second->Deactivate();
			}
			else
			{
				m_menus[m_currentMenu].second->KeyPressEvent(keyEvent);
			}
		}
		else
		{
			switch (keyEvent.keysym.scancode)
			{
			case SDL_SCANCODE_LEFT:
				if (m_currentMenu > 0)
				{
					m_currentMenu--;
				}
				break;
			case SDL_SCANCODE_RIGHT:
				if (m_currentMenu < static_cast<long>(m_menus.size()) - 1)
				{
					m_currentMenu++;
				}
				break;
			case SDL_SCANCODE_DOWN:
				m_subMenuHasFocus = true;
				m_menus[m_currentMenu].second->Activate();
				break;
			default: break;
			}
		}
	}
}
