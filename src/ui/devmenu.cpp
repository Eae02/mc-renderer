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
			if (m_selectedEntry == static_cast<long>(i))
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
	
	void DevMenu::KeyPressEvent(const SDL_KeyboardEvent& keyEvent)
	{
		switch (keyEvent.keysym.scancode)
		{
		case SDL_SCANCODE_UP:
			if (m_selectedEntry > 0)
			{
				m_selectedEntry--;
			}
			break;
		case SDL_SCANCODE_DOWN:
			if (m_selectedEntry < static_cast<long>(m_entries.size()) - 1)
			{
				m_selectedEntry++;
			}
			break;
		case SDL_SCANCODE_SPACE:
		case SDL_SCANCODE_RETURN:
			if (const ValueEntry<bool>* valueEntryB = std::get_if<ValueEntry<bool>>(&m_entries[m_selectedEntry].second))
			{
				valueEntryB->m_setValue(!valueEntryB->m_getValue());
			}
			else if (const ActionCallback* actionEntry = std::get_if<ActionCallback>(&m_entries[m_selectedEntry].second))
			{
				(*actionEntry)();
			}
			break;
		default: break;
		}
	}
}
