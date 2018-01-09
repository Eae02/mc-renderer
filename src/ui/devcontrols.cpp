#include "devcontrols.h"
#include "uidrawlist.h"
#include "font.h"
#include "../inputstate.h"

namespace MCR
{
	namespace Style
	{
		const float framePadding = 10;
		const float buttonPadding = 5;
		const float itemPadding = 6;
		
		const glm::vec4 buttonColorDefault(0.55f, 0.5f, 0.45f, 1.0f);
		const glm::vec4 buttonColorHover(0.91f, 0.53f, 0.12f, 1.0f);
		
		const glm::vec4 textColor(1.0f);
	}
	
	DevControls::DevControls(glm::vec2 topLeft, UIDrawList& drawList, const InputState& inputState)
	    : m_topLeft(topLeft), m_drawList(drawList), m_inputState(inputState),
	      m_nextPosition(topLeft + glm::vec2(Style::framePadding)),
	      m_requiredSize(Style::framePadding * 2, Style::framePadding * 2 - Style::itemPadding)
	{
		
	}
	
	bool DevControls::Button(std::string_view text)
	{
		const glm::vec2 textExtents = Font::GetStandardDev().GetTextExtents(text);
		
		const glm::vec2 buttonSize = textExtents + glm::vec2(Style::buttonPadding * 2);
		const glm::vec2 buttonMax = m_nextPosition + buttonSize;
		
		const bool hovered = RectangleContains(m_nextPosition, buttonMax, m_inputState.GetCursorPos());
		
		m_drawList.AddRectangle(m_nextPosition, buttonMax, hovered ? Style::buttonColorHover : Style::buttonColorDefault);
		
		m_drawList.AddText(Font::GetStandardDev(), text, m_nextPosition + Style::buttonPadding, Style::textColor);
		
		AddControlArea(buttonSize);
		
		return hovered && m_inputState.IsButtonClicked(MouseButtons::Left);
	}
	
	void DevControls::Label(std::string_view text)
	{
		AddControlArea(m_drawList.AddText(Font::GetStandardDev(), text, m_nextPosition, Style::textColor));
	}
	
	void DevControls::AddControlArea(glm::vec2 controlSize)
	{
		m_nextPosition.y += controlSize.y + Style::itemPadding;
		
		m_requiredSize.x = std::max(m_requiredSize.x, controlSize.x + Style::framePadding * 2);
		m_requiredSize.y += controlSize.y + Style::itemPadding;
	}
}
