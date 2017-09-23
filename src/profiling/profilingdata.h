#pragma once

#include <chrono>
#include <string_view>
#include <vector>

#include "timertypes.h"

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
		
		inline explicit ProfilingData(std::vector<Duration> durations)
		    : m_durations(durations) { }
		
		inline std::vector<Duration>::const_iterator DurationsBegin() const
		{
			return m_durations.cbegin();
		}
		
		inline std::vector<Duration>::const_iterator DurationsEnd() const
		{
			return m_durations.cend();
		}
		
	private:
		std::vector<Duration> m_durations;
	};
}
