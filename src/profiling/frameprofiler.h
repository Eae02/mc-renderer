#pragma once

#ifdef MCR_DEBUG

#include <string_view>
#include <chrono>
#include <vector>

#include "timertypes.h"
#include "profilingdata.h"
#include "../vulkan/vk.h"

namespace MCR
{
	class FrameProfiler
	{
	public:
		FrameProfiler();
		
		void NewFrame();
		void Reset(CommandBuffer& commandBuffer);
		
		void AddTimerResult(std::string_view name, TimerTypes type, std::chrono::duration<float, std::milli> duration);
		
		uint32_t BeginGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages, std::string_view name);
		
		void EndGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages, uint32_t id);
		
		uint32_t BeginCPUTimer(std::string_view name);
		
		void EndCPUTimer(uint32_t timerID);
		
		ProfilingData GetData(std::chrono::duration<float, std::milli> frameTime) const;
		
		static constexpr uint32_t MaxGPUTimers = 32;
		static constexpr uint32_t MaxCPUTimers = 32;
		
	private:
		struct TimerEntry
		{
			std::string_view m_name;
			TimerTypes m_type;
			int m_index;
			std::chrono::high_resolution_clock::time_point m_cpuStartTime;
			std::chrono::duration<float, std::milli> m_duration;
		};
		
		std::array<TimerEntry, MaxCPUTimers + MaxGPUTimers> m_timers;
		uint32_t m_numUsedTimers = 0;
		
		VkHandle<VkQueryPool> m_queryPool;
		uint32_t m_numUsedGPUTimers = 0;
	};
	
	extern FrameProfiler* currentFrameProfiler;
	
	// ** Create and end timers on the current frame's profiling data. **
	inline uint32_t BeginGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages,
	                              std::string_view name)
	{
		return currentFrameProfiler->BeginGPUTimer(commandBuffer, waitStages, name);
	}
	
	inline void EndGPUTimer(CommandBuffer& commandBuffer, VkPipelineStageFlagBits waitStages, uint32_t id)
	{
		currentFrameProfiler->EndGPUTimer(commandBuffer, waitStages, id);
	}
	
	inline uint32_t BeginCPUTimer(std::string_view name)
	{
		return currentFrameProfiler->BeginCPUTimer(name);
	}
	
	inline void EndCPUTimer(uint32_t timerID)
	{
		currentFrameProfiler->EndCPUTimer(timerID);
	}
}

#else

#include "../vulkan/vk.h"

namespace MCR
{
	inline uint32_t BeginGPUTimer(CommandBuffer&, VkPipelineStageFlagBits, std::string_view)
	{
		return 0;
	}
	
	inline void EndGPUTimer(CommandBuffer&, VkPipelineStageFlagBits, uint32_t)
	{
		
	}
	
	inline uint32_t BeginCPUTimer(std::string_view)
	{
		return 0;
	}
	
	inline void EndCPUTimer(uint32_t)
	{
		
	}
}

#endif
