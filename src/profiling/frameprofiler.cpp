#include "frameprofiler.h"

#ifdef MCR_DEBUG

namespace MCR
{
	FrameProfiler* currentFrameProfiler = nullptr;
	
	FrameProfiler::FrameProfiler()
	    : m_queryPool(CreateQueryPool(VK_QUERY_TYPE_TIMESTAMP, MaxGPUTimers * 2))
	{
		
	}
	
	void FrameProfiler::NewFrame()
	{
		m_numUsedCPUTimers = 0;
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
	
	uint32_t FrameProfiler::BeginGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages,
	                                      std::string_view name)
	{
		commandBuffer.WriteTimestamp(waitStages, *m_queryPool, m_numUsedGPUTimers * 2);
		
		m_timers[m_numUsedTimers] = { name, TimerTypes::GPU, m_numUsedGPUTimers++ };
		return m_numUsedTimers++;
	}
	
	void FrameProfiler::EndGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages, uint32_t timerID)
	{
		commandBuffer.WriteTimestamp(waitStages, *m_queryPool, m_timers[timerID].m_index * 2 + 1);
	}
	
	uint32_t FrameProfiler::BeginCPUTimer(std::string_view name)
	{
		m_cpuTimerStarts[m_numUsedCPUTimers] = std::chrono::high_resolution_clock::now();
		
		m_timers[m_numUsedTimers] = { name, TimerTypes::CPU, m_numUsedCPUTimers++ };
		return m_numUsedTimers++;
	}
	
	void FrameProfiler::EndCPUTimer(uint32_t timerID)
	{
		uint32_t index = m_timers[timerID].m_index;
		
		m_cpuTimerDurations[index] = std::chrono::high_resolution_clock::now() - m_cpuTimerStarts[index];
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
			uint32_t index = m_timers[i].m_index;
			
			durations[i].m_name = m_timers[i].m_name;
			durations[i].m_type = m_timers[i].m_type;
			
			using Duration = std::chrono::duration<float, std::milli>;
			
			if (m_timers[i].m_type == TimerTypes::CPU)
			{
				durations[i].m_duration = Duration(m_cpuTimerDurations[index].count() * 1E-6f);
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
