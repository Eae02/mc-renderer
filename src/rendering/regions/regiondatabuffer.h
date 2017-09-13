#pragma once

#include "../../vulkan/vk.h"

#include <mutex>
#include <vector>

namespace MCR
{
	class RegionDataBuffer final
	{
	public:
		inline RegionDataBuffer()
		{
			m_desc.m_buffer = VK_NULL_HANDLE;
		}
		
		inline RegionDataBuffer(RegionDataBuffer&& other)
		    : m_desc(std::move(other.m_desc))
		{
			other.m_desc.m_buffer = VK_NULL_HANDLE;
		}
		
		~RegionDataBuffer();
		
		RegionDataBuffer(const RegionDataBuffer& other) = delete;
		
		inline RegionDataBuffer& operator=(RegionDataBuffer&& other)
		{
			this->~RegionDataBuffer();
			m_desc = std::move(other.m_desc);
			other.m_desc.m_buffer = VK_NULL_HANDLE;
			return *this;
		}
		
		inline VkBuffer GetBuffer() const
		{
			return m_desc.m_buffer;
		}
		
		void Upload(CommandBuffer& commandBuffer, VkBuffer source, uint64_t size, VkFence fence);
		
		void PrepareForDraw(CommandBuffer& commandBuffer);
		
		static RegionDataBuffer Allocate(uint64_t size);
		
		static void DestroyResources();
		
	private:
		static CommandBuffer RecordGraphicsReleaseCB(VkBuffer buffer);
		
		struct Description
		{
			uint64_t m_size;
			VkBuffer m_buffer;
			VmaAllocation m_allocation;
			
			bool m_firstUse;
			CommandBuffer m_graphicsReleaseCB;
			VkHandle<VkSemaphore> m_graphicsToTransferSemaphore;
		};
		
		static VkHandle<VkCommandPool> s_queueTransferCommandPool;
		
		static std::mutex s_mutex;
		static std::vector<Description> s_available;
		
		inline explicit RegionDataBuffer(Description description)
		    : m_desc(std::move(description)) { }
		
		bool m_needsGraphicsAquire = false;
		
		Description m_desc;
	};
}
