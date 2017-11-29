#include "devmenu.h"
#include "uidrawlist.h"
#include "font.h"

namespace MCR
{
	const glm::vec3 DevMenu::BackgroundColor(0.22f, 0.2f, 0.25f);
	const glm::vec3 DevMenu::TextColor(0.9f, 0.9f, 0.9f);
	
	template <typename T>
	std::string EntryValueToString(T value)
	{
		return std::to_string(value);
	}
	
	template <> std::string EntryValueToString(bool value)
	{
		return value ? "true": "false";
	}
	
	void DevMenu::Render(UIDrawList& drawList, const glm::vec2& position) const
	{
		const Font& font = Font::GetStandardDev();
		
		float width = 200;
		for (const auto& entry : m_entries)
		{
			width = std::max(width, GetEntryWidth(font, entry.first, entry.second));
		}
		
		const float padding = 4;
		const float itemHeight = font.GetSize() + padding * 2;
		const float height = m_entries.size() * itemHeight;
		
		drawList.AddRectangle(position, position + glm::vec2(width, height), glm::vec4(BackgroundColor, 0.9f));
		
		float y = position.y;
		for (size_t i = 0; i < m_entries.size(); i++)
		{
			bool selected = m_selectedEntry == static_cast<long>(i);
			if (selected)
			{
				drawList.AddRectangle(glm::vec2(position.x, y), glm::vec2(position.x + width, y + itemHeight),
				                      glm::vec4(1, 1, 1, 0.2f));
			}
			
			const float centerY = y + itemHeight / 2;
			
			//Draws the entry label.
			drawList.AddText(font, m_entries[i].first, glm::vec2(position.x + padding, centerY),
			                 glm::vec4(TextColor, 1), TextPosX::Right, TextPosY::Center);
			
			//Draws the entry value, if it has one.
			std::string valueText;
			
			if (const ValueEntry<bool>* valueEntryB = std::get_if<ValueEntry<bool>>(&m_entries[i].second))
			{
				valueText = EntryValueToString(valueEntryB->m_getValue());
			}
			else if (const ValueEntry<float>* valueEntryF = std::get_if<ValueEntry<float>>(&m_entries[i].second))
			{
				valueText = EntryValueToString(valueEntryF->m_getValue());
			}
			else if (const std::unique_ptr<DevMenu>* subMenuPtr =
			         std::get_if<std::unique_ptr<DevMenu>>(&m_entries[i].second))
			{
				float triangleMaxX = position.x + width - padding;
				float triangleSize = 6;
				glm::vec2 trianglePositions[] = 
				{
					{ triangleMaxX - triangleSize, centerY + triangleSize },
					{ triangleMaxX, centerY },
					{ triangleMaxX - triangleSize, centerY - triangleSize }
				};
				
				drawList.AddTriangle(trianglePositions, glm::vec4(TextColor, 1));
				
				if (selected)
				{
					(**subMenuPtr).Render(drawList, glm::vec2(position.x + width, y));
				}
			}
			
			if (!valueText.empty())
			{
				drawList.AddText(font, valueText, glm::vec2(position.x + width - padding, centerY),
				                 glm::vec4(TextColor, 0.85f), TextPosX::Left, TextPosY::Center);
			}
			
			y += itemHeight;
		}
	}
	
	float DevMenu::GetEntryWidth(const Font& font, std::string_view name, const DevMenu::EntryVariant& entryVariant)
	{
		float width = font.GetTextExtents(name).x;
		
		const float nameValueSpacing = 25;
		
		if (const ValueEntry<bool>* valueEntryB = std::get_if<ValueEntry<bool>>(&entryVariant))
		{
			width += font.GetTextExtents(EntryValueToString(valueEntryB->m_getValue())).x + nameValueSpacing;
		}
		else if (const ValueEntry<float>* valueEntryF = std::get_if<ValueEntry<float>>(&entryVariant))
		{
			width += font.GetTextExtents(EntryValueToString(valueEntryF->m_getValue())).x + nameValueSpacing;
		}
		
		return width;
	}
	
	bool DevMenu::KeyPressEvent(const SDL_KeyboardEvent& keyEvent)
	{
		EntryVariant* selectedVariantPtr = &m_entries[m_selectedEntry].second;
		
		DevMenu* selectedEntrySubMenu = [&] () -> DevMenu*
		{
			if (auto subMenuPtr = std::get_if<std::unique_ptr<DevMenu>>(selectedVariantPtr))
				return subMenuPtr->get();
			return nullptr;
		}();
		
		if (m_subMenuHasFocus)
		{
			if (selectedEntrySubMenu->KeyPressEvent(keyEvent))
			{
				return true;
			}
			
			if (keyEvent.keysym.scancode == SDL_SCANCODE_LEFT)
			{
				m_subMenuHasFocus = false;
				selectedEntrySubMenu->Deactivate();
				return true;
			}
			
			return false;
		}
		
		switch (keyEvent.keysym.scancode)
		{
		case SDL_SCANCODE_UP:
			if (m_selectedEntry > 0)
			{
				m_selectedEntry--;
				return true;
			}
			break;
		case SDL_SCANCODE_DOWN:
			if (m_selectedEntry < static_cast<long>(m_entries.size()) - 1)
			{
				m_selectedEntry++;
				return true;
			}
			break;
		case SDL_SCANCODE_LEFT:
			if (const ValueEntry<float>* valueEntryF = std::get_if<ValueEntry<float>>(selectedVariantPtr))
			{
				valueEntryF->m_setValue(valueEntryF->m_getValue() - 0.1f);
				return true;
			}
			break;
		case SDL_SCANCODE_RIGHT:
			if (const ValueEntry<float>* valueEntryF = std::get_if<ValueEntry<float>>(selectedVariantPtr))
			{
				valueEntryF->m_setValue(valueEntryF->m_getValue() + 0.1f);
				return true;
			}
			if (selectedEntrySubMenu != nullptr)
			{
				m_subMenuHasFocus = true;
				selectedEntrySubMenu->Activate();
				return true;
			}
			break;
		case SDL_SCANCODE_SPACE:
		case SDL_SCANCODE_RETURN:
			if (const ValueEntry<bool>* valueEntryB = std::get_if<ValueEntry<bool>>(selectedVariantPtr))
			{
				valueEntryB->m_setValue(!valueEntryB->m_getValue());
				return true;
			}
			if (const ActionCallback* actionEntry = std::get_if<ActionCallback>(selectedVariantPtr))
			{
				(*actionEntry)();
				return true;
			}
			if (selectedEntrySubMenu != nullptr)
			{
				m_subMenuHasFocus = true;
				selectedEntrySubMenu->Activate();
				return true;
			}
			break;
		default: break;
		}
		
		return false;
	}
}
