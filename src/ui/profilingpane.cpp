#include "profilingpane.h"
#include "uidrawlist.h"
#include "devcontrols.h"
#include "font.h"
#include "../inputstate.h"
#include "../profiling/profilingdata.h"

#include <algorithm>
#include <fstream>
#include <sstream>

#ifdef MCR_DEBUG
namespace MCR
{
	ProfilingPane::ProfilingPane()
	    : m_position(100)
	{
		m_recordedFrames.resize(NumFramesToKeep);
	}
	
	const float width = 300;
	const float titleHeight = 24;
	
	void ProfilingPane::FrameEnd(ProfilingData profilingData, const InputState& inputState,
	                             UIDrawList& drawList)
	{
		if (m_visible)
		{
			Render(profilingData, inputState, drawList);
			
			if (!m_isMoving && RectangleContains(m_position, m_position + glm::vec2(width, titleHeight),
			                                     inputState.GetCursorPos()) &&
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
		
		m_totalProfilingDataTime = m_totalProfilingDataTime + profilingData.GetFrameTime() -
		        m_recordedFrames[m_nextProfilingDataIndex].GetFrameTime();
		
		m_recordedFrames[m_nextProfilingDataIndex] = std::move(profilingData);
		
		if (m_numRecordedFrames < NumFramesToKeep)
		{
			m_numRecordedFrames++;
		}
		
		m_nextProfilingDataIndex = (m_nextProfilingDataIndex + 1) % NumFramesToKeep;
		
		if (inputState.IsKeyPressed(Keys::F4) && !inputState.WasKeyPressed(Keys::F4))
		{
			DumpToCSV();
		}
	}
	
	void ProfilingPane::DumpToCSV()
	{
		std::ostringstream nameStream;
		nameStream << "./frame_prof_" << frameIndex << ".csv";
		std::string name = nameStream.str();
		
		std::ofstream stream(name);
		
		const ProfilingData& firstFrameData = m_nextProfilingDataIndex == 0 ?
		            m_recordedFrames[m_nextProfilingDataIndex - 1] : m_recordedFrames.back();
		
		stream << "Frame Time";
		std::for_each(firstFrameData.DurationsBegin(), firstFrameData.DurationsEnd(), [&] (const ProfilingData::Duration& d)
		{
			stream << "," << d.m_name;
		});
		stream << "\n";
		
		IterateRecordedFrames([&] (const ProfilingData& profilingData)
		{
			stream << profilingData.GetFrameTime().count();
			std::for_each(firstFrameData.DurationsBegin(), firstFrameData.DurationsEnd(),
			              [&] (const ProfilingData::Duration& firstFrameDuration)
			{
				auto it = std::find_if(profilingData.DurationsBegin(), profilingData.DurationsEnd(),
				                       [&] (const ProfilingData::Duration& duration)
				{
					return duration.m_name == firstFrameDuration.m_name;
				});
				
				if (it == profilingData.DurationsEnd())
				{
					stream << ",";
				}
				else
				{
					stream << "," << it->m_duration.count();
				}
			});
			
			stream << "\n";
		});
	}
	
	void ProfilingPane::Render(const ProfilingData& profilingData, const InputState& inputState, UIDrawList& drawList)
	{
		const glm::vec4 backgroundColor(0.1f, 0.1f, 0.1f, 0.75f);
		
		const glm::vec4 titleBackgroundColor(0.8f, 0.45f, 0.3f, 1.0f);
		
		glm::vec2 titleRectMax = m_position + glm::vec2(width, titleHeight);
		
		//Draws the title
		drawList.AddRectangle(m_position, titleRectMax, titleBackgroundColor);
		
		drawList.AddText(Font::GetStandardDev(), "Profiling", m_position + glm::vec2(5, titleHeight / 2),
		                 glm::vec4(1.0f), TextPosX::Right, TextPosY::Center);
		
		m_contentsList.Reset();
		
		glm::vec2 pos = m_position + glm::vec2(0, titleHeight);
		
		std::ostringstream topTextStream;
		topTextStream << "Frame Time: " << profilingData.GetFrameTime().count() << "ms\n";
		topTextStream << "FPS: " << (1000.0f / profilingData.GetFrameTime().count()) << "Hz";
		const std::string topText = topTextStream.str();
		
		pos.y += m_contentsList.AddText(Font::GetStandardDev(), topText, pos + glm::vec2(5.0f), glm::vec4(1.0f),
		                                TextPosX::Right, TextPosY::Below).y + 10.0f;
		
		RenderPane(pos, "Current Frame (CPU)", inputState, m_currentFramePaneOpen, [&] (DevControls& devControls)
		{
			RenderPane_CurrentFrame(devControls, profilingData, TimerTypes::CPU);
		});
		
		RenderPane(pos, "Current Frame (GPU)", inputState, m_currentFramePaneOpen, [&] (DevControls& devControls)
		{
			RenderPane_CurrentFrame(devControls, profilingData, TimerTypes::GPU);
		});
		
		RenderPane(pos, "Graphs", inputState, m_graphsPaneOpen, [&] (DevControls& devControls)
		{
			RenderPane_Graphs(devControls);
		});
		
		drawList.AddRectangle(m_position + glm::vec2(0, titleHeight), pos + glm::vec2(width, 0), backgroundColor);
		
		drawList.AddList(m_contentsList);
	}
	
	void ProfilingPane::RenderPaneHeader(UIDrawList& drawList, const InputState& inputState, glm::vec2& pos,
	                                     std::string_view name, bool& open)
	{
		const float headerHeight = 20;
		const float margin = 5;
		const glm::vec4 headerBackgroundColor(0.8f, 0.45f, 0.3f, 0.8f);
		const glm::vec4 headerBackgroundColorHover(0.8f, 0.45f, 0.3f, 1.0f);
		
		glm::vec2 rectMin = pos + glm::vec2(margin);
		glm::vec2 rectMax = rectMin + glm::vec2(width - margin * 2, headerHeight);
		
		bool hovered = RectangleContains(rectMin, rectMax, inputState.GetCursorPos());
		if (hovered && inputState.IsButtonClicked(MouseButtons::Left))
			open = !open;
		
		drawList.AddRectangle(rectMin, rectMax, hovered ? headerBackgroundColorHover : headerBackgroundColor);
		
		const float triangleMargin = headerHeight * 0.3f;
		const float triangleHeight = headerHeight - triangleMargin * 2;
		const float triangleSideLen = std::sqrt(0.75f) * triangleHeight * 2;
		
		drawList.AddText(Font::GetStandardDev(), name, rectMin + glm::vec2(headerHeight, headerHeight / 2),
		                 glm::vec4(1.0f), TextPosX::Right, TextPosY::Center);
		
		std::array<glm::vec2, 3> trianglePositions;
		const glm::vec2 trianglePos = rectMin + glm::vec2(triangleMargin);
		trianglePositions[0] = trianglePos;
		trianglePositions[1] = trianglePos + glm::vec2(triangleSideLen * 0.4f, triangleHeight);
		trianglePositions[2] = trianglePos + glm::vec2(triangleSideLen * 0.8f, 0);
		
		drawList.AddTriangle(trianglePositions.data(), glm::vec4(1.0f));
		
		pos.y += headerHeight + margin;
	}
	
	void ProfilingPane::RenderPane_CurrentFrame(DevControls& devControls, const ProfilingData& profilingData,
	                                            TimerTypes timerType)
	{
		std::ostringstream labelStream;
		labelStream << std::fixed << std::setprecision(4);
		
		std::for_each(profilingData.DurationsBegin(), profilingData.DurationsEnd(),
		              [&] (const ProfilingData::Duration& duration)
		{
			if (duration.m_type == timerType)
			{
				labelStream << duration.m_name << ": " << duration.m_duration.count() << "ms\n";
			}
		});
		
		std::string labelString = labelStream.str();
		devControls.Label(labelString);
	}
	
	void ProfilingPane::RenderPane_Graphs(DevControls& devControls)
	{
		if (m_recordedFrames.empty())
			return;
		
		using DurationType = std::chrono::duration<float, std::milli>;
		
		std::vector<DurationType> durationsBuffer;
		durationsBuffer.reserve(NumFramesToKeep);
		
		const std::string_view graphs[] = 
		{
			"GPU sync", "Render List Fill"
		};
		
		for (std::string_view graphName : graphs)
		{
			devControls.Label(graphName);
			
			devControls.CustomControl([&] (UIDrawList& drawList, const InputState& inputState, glm::vec2 pos)
			{
				glm::vec2 size(width - 20, 150);
				
				drawList.AddRectangle(pos, pos + size, glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
				
				durationsBuffer.clear();
				
				float durationSum = 0;
				int durationCount = 0;
				
				DurationType accumulatedFrameTime = DurationType::zero();
				
				IterateRecordedFrames([&] (const ProfilingData& profilingData)
				{
					auto it = std::find_if(profilingData.DurationsBegin(), profilingData.DurationsEnd(),
					                       [&] (const ProfilingData::Duration& d2)
					{
						return d2.m_name == graphName;
					});
					
					if (it == profilingData.DurationsEnd())
						return;
					
					durationSum += it->m_duration.count();
					durationCount++;
					
					durationsBuffer.push_back(it->m_duration);
					
					accumulatedFrameTime += profilingData.GetFrameTime();
				});
				
				glm::vec3 lineColor(0, 1, 0);
				
				float avgDuration = durationSum / durationCount;
				float maxDuration = avgDuration * 3.0f;
				
				const float lineAlpha = 0.3f;
				
				float increment = std::pow(10, std::floor(std::log10(maxDuration / 5.0f))) * 5.0f;
				for (float i = 0; i < maxDuration; i += increment)
				{
					std::ostringstream stream;
					stream << i;
					std::string valueStr = stream.str();
					
					float y = pos.y + size.y * (1.0f - i / maxDuration);
					
					glm::vec2 textSize = drawList.AddText(Font::GetStandardDev(), valueStr, glm::vec2(pos.x + 3, y),
					                                      glm::vec4(lineColor, lineAlpha), TextPosX::Right, TextPosY::Center);
					
					drawList.AddLine(glm::vec2(pos.x + 6 + textSize.x, y), glm::vec2(pos.x + size.x - 3, y),
					                 glm::vec4(lineColor, lineAlpha));
				}
				
				const float dx = size.x / NumFramesToKeep;
				
				for (size_t i = 1; i < durationsBuffer.size(); i++)
				{
					float prevY = durationsBuffer[i - 1].count() / maxDuration;
					float thisY = durationsBuffer[i].count() / maxDuration;
					
					drawList.AddLine(pos + glm::vec2(dx * (i - 1), size.y * std::max(1.0f - prevY, 0.0f)),
					                 pos + glm::vec2(dx * i, size.y * std::max(1.0f - thisY, 0.0f)), glm::vec4(0, 1, 0, 1));
				}
				
				return size;
			});
		}
	}
}
#endif
