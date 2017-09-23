#pragma once

#include <glm/glm.hpp>
#include <string_view>

namespace MCR
{
	class DevControls
	{
	public:
		DevControls(glm::vec2 topLeft, class UIDrawList& drawList, const class InputState& inputState);
		
		bool Button(std::string_view text);
		
		void Label(std::string_view text);
		
		inline glm::vec2 GetRequiredSize() const
		{
			return m_requiredSize;
		}
		
	private:
		void AddControlArea(glm::vec2 controlSize);
		
		const glm::vec2 m_topLeft;
		
		class UIDrawList& m_drawList;
		const class InputState& m_inputState;
		
		glm::vec2 m_nextPosition;
		glm::vec2 m_requiredSize;
	};
}
