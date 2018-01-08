#pragma once

#include <chrono>
#include <string_view>
#include <vector>

#include "timertypes.h"

#ifndef NO_PROF
namespace MCR
{
	class ProfilingData
	{
	public:
		struct Duration
		{
			std::chrono::duration<float, std::milli> m_duration;
			std::string_view m_name;
			TimerTypes m_type;
		};
		
		ProfilingData() = default;
		
		inline ProfilingData(std::vector<Duration> durations, std::chrono::duration<float, std::milli> frameTime)
		    : m_durations(durations), m_frameTime(frameTime) { }
		
		inline std::vector<Duration>::const_iterator DurationsBegin() const
		{
			return m_durations.cbegin();
		}
		
		inline std::vector<Duration>::const_iterator DurationsEnd() const
		{
			return m_durations.cend();
		}
		
		inline std::chrono::duration<float, std::milli> GetFrameTime() const
		{
			return m_frameTime;
		}
		
	private:
		std::vector<Duration> m_durations;
		std::chrono::duration<float, std::milli> m_frameTime;
	};
}
#endif
