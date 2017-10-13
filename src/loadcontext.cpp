#include "loadcontext.h"

namespace MCR
{
	LoadContext::LoadContext()
	    : m_commandPool(CreateCommandPool(QUEUE_FAMILY_GRAPHICS, 0)), m_commandBuffer(*m_commandPool)
	{
		
	}
	
	void LoadContext::Begin()
	{
		m_commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	}
	
	void LoadContext::End()
	{
		m_commandBuffer.End();
		
		VkHandle<VkFence> fence = CreateVkFence();
		
		VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_commandBuffer.GetVkCB();
		vulkan.queues[QUEUE_FAMILY_GRAPHICS]->Submit(1, &submitInfo, *fence);
		
		CheckResult(vkWaitForFences(vulkan.device, 1, &*fence, true, UINT64_MAX));
		
		for (const Resource& resource : m_resources)
		{
			resource.m_destroy(resource.m_resource);
		}
		m_resources.clear();
	}
}
