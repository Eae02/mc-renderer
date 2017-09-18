#pragma once

#include <vulkan/vulkan.h>
#include <mutex>

#include "func.h"

namespace MCR
{
	class Queue
	{
	public:
		Queue(uint32_t familyIndex, uint32_t index);
		
		void Submit(uint32_t submitCount, const VkSubmitInfo* submitInfos, VkFence fence = VK_NULL_HANDLE);
		
		VkResult Present(const VkPresentInfoKHR& presentInfo);
		
		inline uint32_t GetFamilyIndex() const
		{ return m_familyIndex; }
		inline uint32_t GetIndex() const
		{ return m_index; }
		
		inline void WaitIdle()
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			vkQueueWaitIdle(m_queue);
		}
		
	private:
		VkQueue m_queue;
		
		std::mutex m_mutex;
		
		uint32_t m_familyIndex;
		uint32_t m_index;
	};
}
