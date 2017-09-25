#include "profilingpane.h"
#include "uidrawlist.h"
#include "devcontrols.h"
#include "font.h"
#include "../inputstate.h"
#include "../profiling/profilingdata.h"

#include <algorithm>
#include <sstream>

#ifdef MCR_DEBUG
namespace MCR
{
	void ProfilingPane::FrameEnd(ProfilingData profilingData, const InputState& inputState,
	                             UIDrawList& drawList)
	{
		const glm::vec4 backgroundColor(0.1f, 0.1f, 0.1f, 0.75f);
		
		const float titleHeight = 24;
		const glm::vec4 titleBackgroundColor(0.8f, 0.45f, 0.3f, 1.0f);
		
		m_contentsList.Reset();
		
		glm::vec2 contentMin = m_position + glm::vec2(0, titleHeight);
		DevControls devControls(contentMin, m_contentsList, inputState);
		
		std::for_each(profilingData.DurationsBegin(), profilingData.DurationsEnd(),
		              [&] (const ProfilingData::Duration& duration)
		{
			std::ostringstream labelStream;
			labelStream << duration.m_name << ": " << std::fixed << std::setprecision(4) <<
			               duration.m_duration.count() << "ms";
			
			if (duration.m_type == TimerTypes::CPU)
				labelStream << " [CPU]";
			else
				labelStream << " [GPU]";
			
			std::string labelString = labelStream.str();
			
			devControls.Label(labelString);
		});
		
		glm::vec2 size = devControls.GetRequiredSize();
		if (size.x < 300)
			size.x = 300;
		
		glm::vec2 titleRectMax = m_position + glm::vec2(size.x, titleHeight);
		
		drawList.AddRectangle(m_position, titleRectMax, titleBackgroundColor);
		
		drawList.AddText(Font::GetStandardDev(), "Profiling", m_position + glm::vec2(5, titleHeight / 2),
		                 glm::vec4(1.0f), TextPosX::Right, TextPosY::Center);
		
		drawList.AddRectangle(contentMin, contentMin + size, backgroundColor);
		
		drawList.AddList(m_contentsList);
		
		if (!m_isMoving && RectangleContains(m_position, titleRectMax, inputState.GetCursorPos()) &&
		    inputState.IsButtonPressed(MouseButtons::Left) && !inputState.WasButtonPressed(MouseButtons::Left))
		{
			m_isMoving = true;
		}
		
		if (m_isMoving)
		{
			if (inputState.IsButtonPressed(MouseButtons::Left))
			{
				m_position += inputState.GetCursorDelta();
			}
			else
			{
				m_isMoving = false;
			}
		}
	}
}
#endif
