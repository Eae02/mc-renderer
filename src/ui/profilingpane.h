#pragma once

#include <glm/glm.hpp>
#include <chrono>

#include "uidrawlist.h"
#include "devcontrols.h"
#include "../inputstate.h"
#include "../profiling/profilingdata.h"

namespace MCR
{
#ifndef NO_PROF
	class ProfilingPane
	{
	public:
		ProfilingPane();
		
		void FrameEnd(ProfilingData profilingData, const class InputState& inputState,
		              UIDrawList& drawList);
		
		inline bool IsVisible() const
		{
			return m_visible;
		}
		
		inline void SetVisible(bool visible)
		{
			m_visible = visible;
		}
		
		void DumpToCSV();
		
	private:
		void Render(const ProfilingData& profilingData, const InputState& inputState, UIDrawList& drawList);
		
		template <typename CallbackTp>
		inline void IterateRecordedFrames(CallbackTp callback) const
		{
			for (long i = 1; i <= static_cast<long>(m_numRecordedFrames); i++)
			{
				long realIndex = static_cast<long>(m_nextProfilingDataIndex) - i;
				if (realIndex < 0)
				{
					realIndex += NumFramesToKeep;
				}
				
				callback(m_recordedFrames[realIndex]);
			}
		}
		
		template <typename CallbackTp>
		inline void RenderPane(glm::vec2& pos, std::string_view name, const class InputState& inputState,
		                       bool& open, CallbackTp callback)
		{
			RenderPaneHeader(m_contentsList, inputState, pos, name, open);
			
			if (open)
			{
				DevControls devControls(pos + glm::vec2(5, 0), m_contentsList, inputState);
				callback(devControls);
				pos.y += devControls.GetRequiredSize().y;
			}
		}
		
		void RenderPaneHeader(UIDrawList& drawList, const InputState& inputState, glm::vec2& pos,
		                      std::string_view name, bool& open);
		
		void RenderPane_CurrentFrame(class DevControls& devControls, const ProfilingData& profilingData,
		                             TimerTypes timerType);
		void RenderPane_Graphs(class DevControls& devControls);
		
		static constexpr size_t NumFramesToKeep = 200;
		
		bool m_currentFramePaneOpen = true;
		bool m_graphsPaneOpen = true;
		
		std::chrono::duration<float, std::milli> m_totalProfilingDataTime =
		        std::chrono::duration<float, std::milli>::zero();
		
		size_t m_numRecordedFrames = 0;
		size_t m_nextProfilingDataIndex = 0;
		std::vector<ProfilingData> m_recordedFrames;
		
		glm::vec2 m_position;
		
		UIDrawList m_contentsList;
		
		bool m_visible = false;
		bool m_isMoving = false;
	};
#endif
}
