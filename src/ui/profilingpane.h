#pragma once

#include <glm/glm.hpp>

#include "../profiling/profilingdata.h"
#include "uidrawlist.h"

namespace MCR
{
	class ProfilingPane
	{
	public:
		ProfilingPane()
		    : m_position(100) { }
		
		void FrameEnd(ProfilingData profilingData, const class InputState& inputState,
		              class UIDrawList& drawList);
		
	private:
		glm::vec2 m_position;
		
		UIDrawList m_contentsList;
		
		bool m_isMoving = false;
	};
}
