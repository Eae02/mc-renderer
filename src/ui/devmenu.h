#pragma once

#include <variant>
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <glm/glm.hpp>
#include <SDL2/SDL_events.h>

namespace MCR
{
	class DevMenu
	{
	public:
		template <typename T>
		using GetValueCallback = std::function<T()>;
		template <typename T>
		using SetValueCallback = std::function<void(T)>;
		
		using ActionCallback = std::function<void()>;
		
		DevMenu() = default;
		
		void Render(class UIDrawList& drawList, const glm::vec2& position) const;
		
		bool KeyPressEvent(const SDL_KeyboardEvent& keyEvent);
		
		template <typename T>
		inline void AddValue(std::string_view name, GetValueCallback<T> get, SetValueCallback<T> set)
		{
			m_entries.push_back(std::make_pair(name, EntryVariant(ValueEntry<T>{ std::move(get), std::move(set) })));
		}
		
		inline void AddAction(std::string_view name, ActionCallback callback)
		{
			m_entries.push_back(std::make_pair(name, EntryVariant(std::move(callback))));
		}
		
		inline void AddSubMenu(std::string_view name, std::unique_ptr<DevMenu> subMenu)
		{
			m_entries.push_back(std::make_pair(name, EntryVariant(std::move(subMenu))));
		}
		
		inline void Activate()
		{
			m_selectedEntry = 0;
		}
		
		inline void Deactivate()
		{
			m_selectedEntry = -1;
		}
		
		inline long GetSelectedEntry() const
		{
			return m_selectedEntry;
		}
		
		static const glm::vec3 BackgroundColor;
		static const glm::vec3 TextColor;
		
	private:
		template <typename T>
		struct ValueEntry
		{
			GetValueCallback<T> m_getValue;
			SetValueCallback<T> m_setValue;
		};
		
		using EntryVariant = std::variant<ValueEntry<bool>, ValueEntry<float>, ActionCallback, std::unique_ptr<DevMenu>>;
		
		static float GetEntryWidth(const class Font& font, std::string_view name, const EntryVariant& entryVariant);
		
		std::vector<std::pair<std::string_view, EntryVariant>> m_entries;
		
		bool m_subMenuHasFocus = false;
		long m_selectedEntry = -1;
	};
}
