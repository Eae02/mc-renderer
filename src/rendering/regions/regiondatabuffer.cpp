#include "regiondatabuffer.h"

#include <algorithm>

namespace MCR
{
	std::mutex RegionDataBuffer::s_mutex;
	std::vector<RegionDataBuffer::Description> RegionDataBuffer::s_available;
	
	VkHandle<VkCommandPool> RegionDataBuffer::s_queueTransferCommandPool;
	
	RegionDataBuffer::~RegionDataBuffer()
	{
		if (m_desc.m_buffer != VK_NULL_HANDLE)
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			s_available.push_back(std::move(m_desc));
		}
	}
	
	inline bool TransferQueueIsGraphicsQueue()
	{
		return vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS] == vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER];
	}
	
	void RegionDataBuffer::Upload(CommandBuffer& commandBuffer, VkBuffer source, uint64_t size, VkFence fence)
	{
		commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
		
		if (!m_desc.m_firstUse)
		{
			if (TransferQueueIsGraphicsQueue())
			{
				//Makes sure that graphics operations have completed before starting the transfer.
				VkBufferMemoryBarrier barrier = 
				{
					/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					/* pNext               */ nullptr,
					/* srcAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT,
					/* dstAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
					/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
					/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
					/* buffer              */ m_desc.m_buffer,
					/* offset              */ 0,
					/* size                */ VK_WHOLE_SIZE
				};
				commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				                              { }, SingleElementSpan(barrier), { });
			}
			else
			{
				//Aquires the data buffer from the graphics queue.
				VkBufferMemoryBarrier barrier = 
				{
					/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
					/* pNext               */ nullptr,
					/* srcAccessMask       */ 0,
					/* dstAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
					/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
					/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
					/* buffer              */ m_desc.m_buffer,
					/* offset              */ 0,
					/* size                */ VK_WHOLE_SIZE
				};
				commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				                              { }, SingleElementSpan(barrier), { });
			}
		}
		
		commandBuffer.CopyBuffer(source, m_desc.m_buffer, { 0, 0, size });
		
		if (TransferQueueIsGraphicsQueue())
		{
			//Makes sure that the transfer has completed before the buffer is used to draw.
			VkBufferMemoryBarrier barrier = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT,
				/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
				/* buffer              */ m_desc.m_buffer,
				/* offset              */ 0,
				/* size                */ VK_WHOLE_SIZE
			};
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			                              { }, SingleElementSpan(barrier), { });
		}
		else
		{
			//Releases the data buffer from the transfer queue.
			VkBufferMemoryBarrier barrier = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
				/* dstAccessMask       */ 0,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* buffer              */ m_desc.m_buffer,
				/* offset              */ 0,
				/* size                */ VK_WHOLE_SIZE
			};
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
			                              { }, SingleElementSpan(barrier), { });
		}
		
		commandBuffer.End();
		
		VkSubmitInfo transferSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		transferSubmitInfo.commandBufferCount = 1;
		transferSubmitInfo.pCommandBuffers = &commandBuffer.GetVkCB();
		
		if (!TransferQueueIsGraphicsQueue())
		{
			if (!m_desc.m_firstUse)
			{
				VkSubmitInfo graphicsReleaseSubmitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
				graphicsReleaseSubmitInfo.commandBufferCount = 1;
				graphicsReleaseSubmitInfo.pCommandBuffers = &m_desc.m_graphicsReleaseCB.GetVkCB();
				graphicsReleaseSubmitInfo.signalSemaphoreCount = 1;
				graphicsReleaseSubmitInfo.pSignalSemaphores = &*m_desc.m_graphicsToTransferSemaphore;
				
				vulkan.queues[QUEUE_FAMILY_GRAPHICS]->Submit(1, &graphicsReleaseSubmitInfo, nullptr);
				
				const VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
				transferSubmitInfo.waitSemaphoreCount = 1;
				transferSubmitInfo.pWaitSemaphores = &*m_desc.m_graphicsToTransferSemaphore;
				transferSubmitInfo.pWaitDstStageMask = &waitDstStageMask;
			}
		}
		
		vulkan.queues[QUEUE_FAMILY_TRANSFER]->Submit(1, &transferSubmitInfo, fence);
		
		m_desc.m_firstUse = false;
	}
	
	void RegionDataBuffer::PrepareForDraw(CommandBuffer& commandBuffer)
	{
		if (m_needsGraphicsAquire)
		{
			const VkBufferMemoryBarrier barrier = 
			{
				/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				/* pNext               */ nullptr,
				/* srcAccessMask       */ 0,
				/* dstAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
				/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
				/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
				/* buffer              */ m_desc.m_buffer,
				/* offset              */ 0,
				/* size                */ VK_WHOLE_SIZE
			};
			commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0,
			                              { }, SingleElementSpan(barrier), { });
			
			m_needsGraphicsAquire = false;
		}
	}
	
	CommandBuffer RegionDataBuffer::RecordGraphicsReleaseCB(VkBuffer buffer)
	{
		CommandBuffer commandBuffer(*s_queueTransferCommandPool);
		
		commandBuffer.Begin(0);
		
		VkBufferMemoryBarrier barrier = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
			/* dstAccessMask       */ 0,
			/* srcQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_GRAPHICS],
			/* dstQueueFamilyIndex */ vulkan.queueFamilies[QUEUE_FAMILY_TRANSFER],
			/* buffer              */ buffer,
			/* offset              */ 0,
			/* size                */ VK_WHOLE_SIZE
		};
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0,
		                              { }, SingleElementSpan(barrier), { });
		
		commandBuffer.End();
		
		return commandBuffer;
	}
	
	RegionDataBuffer RegionDataBuffer::Allocate(uint64_t size)
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		
		ssize_t bestDataBufferIndex = -1;
		for (size_t i = 0; i < s_available.size(); i++)
		{
			if (s_available[i].m_size >= size)
			{
				if (bestDataBufferIndex == -1 || s_available[i].m_size < s_available[bestDataBufferIndex].m_size)
				{
					bestDataBufferIndex = i;
				}
			}
		}
		
		if (bestDataBufferIndex != -1)
		{
			RegionDataBuffer result(std::move(s_available[bestDataBufferIndex]));
			
			if (bestDataBufferIndex != static_cast<ssize_t>(s_available.size()) - 1)
			{
				s_available[bestDataBufferIndex] = std::move(s_available.back());
			}
			s_available.pop_back();
			
			return result;
		}
		
		Description description;
		
		description.m_firstUse = true;
		description.m_size = RoundToNextMultiple<uint64_t>(size, 1024);
		
		const VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
		        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		
		VmaMemoryRequirements memoryRequirements = { };
		memoryRequirements.flags = VMA_MEMORY_REQUIREMENT_OWN_MEMORY_BIT;
		memoryRequirements.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		
		VkBufferCreateInfo bufferCreateInfo;
		InitBufferCreateInfo(bufferCreateInfo, usageFlags, size);
		CheckResult(vmaCreateBuffer(vulkan.allocator, &bufferCreateInfo, &memoryRequirements,
		                            &description.m_buffer, &description.m_allocation, nullptr));
		
		if (!TransferQueueIsGraphicsQueue())
		{
			if (s_queueTransferCommandPool.IsNull())
			{
				s_queueTransferCommandPool = CreateCommandPool(QUEUE_FAMILY_GRAPHICS, 0);
			}
			
			description.m_graphicsReleaseCB = RecordGraphicsReleaseCB(description.m_buffer);
			
			description.m_graphicsToTransferSemaphore = CreateVkSemaphore();
		}
		
		return RegionDataBuffer(std::move(description));
	}
	
	void RegionDataBuffer::DestroyResources()
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		
		for (Description& description : s_available)
		{
			DestroyVulkanObject(description.m_buffer);
			DestroyVulkanObject(description.m_allocation);
		}
		
		s_available.clear();
		
		s_queueTransferCommandPool.Reset();
	}
}
