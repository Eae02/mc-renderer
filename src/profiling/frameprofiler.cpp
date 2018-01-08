#include "frameprofiler.h"

#ifndef NO_PROF

namespace MCR
{
	FrameProfiler* currentFrameProfiler = nullptr;
	
	FrameProfiler::FrameProfiler()
	    : m_queryPool(CreateQueryPool(VK_QUERY_TYPE_TIMESTAMP, MaxGPUTimers * 2))
	{
		
	}
	
	void FrameProfiler::NewFrame()
	{
		m_numUsedGPUTimers = 0;
		m_numUsedTimers = 0;
	}
	
	void FrameProfiler::Reset(CommandBuffer& commandBuffer)
	{
		if (m_numUsedGPUTimers > 0)
		{
			commandBuffer.ResetQueryPool(*m_queryPool, 0, m_numUsedGPUTimers * 2);
		}
	}
	
	void FrameProfiler::AddTimerResult(std::string_view name, TimerTypes type,
	                                   std::chrono::duration<float, std::milli> duration)
	{
		m_timers[m_numUsedTimers++] = { name, type, -1, { }, duration };
	}
	
	uint32_t FrameProfiler::BeginGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages,
	                                      std::string_view name)
	{
		commandBuffer.WriteTimestamp(waitStages, *m_queryPool, m_numUsedGPUTimers * 2);
		
		m_timers[m_numUsedTimers] = { name, TimerTypes::GPU, static_cast<int>(m_numUsedGPUTimers++), { }, { } };
		return m_numUsedTimers++;
	}
	
	void FrameProfiler::EndGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages, uint32_t timerID)
	{
		commandBuffer.WriteTimestamp(waitStages, *m_queryPool, static_cast<uint32_t>(m_timers[timerID].m_index) * 2 + 1);
	}
	
	uint32_t FrameProfiler::BeginCPUTimer(std::string_view name)
	{
		m_timers[m_numUsedTimers] = { name, TimerTypes::CPU, -1, std::chrono::high_resolution_clock::now(), { } };
		return m_numUsedTimers++;
	}
	
	void FrameProfiler::EndCPUTimer(uint32_t timerID)
	{
		m_timers[timerID].m_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
			std::chrono::high_resolution_clock::now() - m_timers[timerID].m_cpuStartTime);
	}
	
	ProfilingData FrameProfiler::GetData(std::chrono::duration<float, std::milli> frameTime) const
	{
		uint64_t gpuTimestamps[MaxGPUTimers * 2];
		
		if (m_numUsedGPUTimers > 0)
		{
			vkGetQueryPoolResults(vulkan.device, *m_queryPool, 0, m_numUsedGPUTimers * 2, sizeof(gpuTimestamps),
			                      gpuTimestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
		}
		
		std::vector<ProfilingData::Duration> durations(m_numUsedTimers);
		
		for (uint32_t i = 0; i < m_numUsedTimers; i++)
		{
			int index = m_timers[i].m_index;
			
			durations[i].m_name = m_timers[i].m_name;
			durations[i].m_type = m_timers[i].m_type;
			
			using Duration = std::chrono::duration<float, std::milli>;
			
			if (m_timers[i].m_index == -1)
			{
				durations[i].m_duration = m_timers[i].m_duration;
			}
			else
			{
				const uint64_t elapsedDeviceTicks = gpuTimestamps[index * 2 + 1] - gpuTimestamps[index * 2];
				durations[i].m_duration = Duration(elapsedDeviceTicks * vulkan.limits.timestampMillisecondPeriod);
			}
		}
		
		return ProfilingData(std::move(durations), frameTime);
	}
}

#endif
