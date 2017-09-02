#include "commandbuffer.h"
#include "instance.h"

namespace MCR
{
	CommandBuffer::CommandBuffer(VkCommandPool pool, VkCommandBufferLevel level)
	    : m_commandPool(pool)
	{
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = pool;
		allocateInfo.level = level;
		allocateInfo.commandBufferCount = 1;
		
		CheckResult(vkAllocateCommandBuffers(vulkan.device, &allocateInfo, &m_commandBuffer));
	}
	
	CommandBuffer::~CommandBuffer()
	{
		if (m_commandBuffer != VK_NULL_HANDLE)
		{
			vkFreeCommandBuffers(vulkan.device, m_commandPool, 1, &m_commandBuffer);
		}
	}
}
