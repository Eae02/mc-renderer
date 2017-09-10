#include "regiondatabuffer.h"

#include <algorithm>

namespace MCR
{
	std::mutex RegionDataBuffer::s_mutex;
	std::vector<RegionDataBuffer::Description> RegionDataBuffer::s_available;
	
	RegionDataBuffer::~RegionDataBuffer()
	{
		if (m_desc.m_buffer != VK_NULL_HANDLE)
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			s_available.push_back(m_desc);
		}
	}
	
	RegionDataBuffer RegionDataBuffer::Allocate(uint64_t size)
	{
		{
			std::lock_guard<std::mutex> lock(s_mutex);
			
			auto it = std::find_if(MAKE_RANGE(s_available), [&] (const Description& desc)
			{
				return desc.m_size >= size;
			});
			
			if (it != s_available.end())
			{
				RegionDataBuffer result(*it);
				
				*it = s_available.back();
				s_available.pop_back();
				
				return result;
			}
		}
		
		Description description;
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
		
		return RegionDataBuffer(description);
	}
	
	void RegionDataBuffer::CollectAvailable()
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		
		for (Description& description : s_available)
		{
			DestroyVulkanObject(description.m_buffer);
			DestroyVulkanObject(description.m_allocation);
		}
		
		s_available.clear();
	}
}
