#include "queue.h"
#include "func.h"
#include "instance.h"
#include "vkutils.h"

namespace MCR
{
	Queue::Queue(uint32_t familyIndex, uint32_t index)
	    : m_familyIndex(familyIndex), m_index(index)
	{
		vkGetDeviceQueue(vulkan.device, familyIndex, index, &m_queue);
	}
	
	void Queue::Submit(uint32_t submitCount, const VkSubmitInfo* submitInfos, VkFence fence)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		CheckResult(vkQueueSubmit(m_queue, submitCount, submitInfos, fence));
	}
	
	VkResult Queue::Present(const VkPresentInfoKHR& presentInfo)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		
		return vkQueuePresentKHR(m_queue, &presentInfo);
	}
}
