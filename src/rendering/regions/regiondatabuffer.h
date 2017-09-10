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
		    : m_desc(other.m_desc)
		{
			other.m_desc.m_buffer = VK_NULL_HANDLE;
		}
		
		~RegionDataBuffer();
		
		RegionDataBuffer(const RegionDataBuffer& other) = delete;
		
		inline RegionDataBuffer& operator=(RegionDataBuffer&& other)
		{
			this->~RegionDataBuffer();
			m_desc = other.m_desc;
			other.m_desc.m_buffer = VK_NULL_HANDLE;
			return *this;
		}
		
		inline VkBuffer GetBuffer() const
		{
			return m_desc.m_buffer;
		}
		
		static RegionDataBuffer Allocate(uint64_t size);
		
		static void CollectAvailable();
		
	private:
		struct Description
		{
			uint64_t m_size;
			VkBuffer m_buffer;
			VmaAllocation m_allocation;
		};
		
		static std::mutex s_mutex;
		static std::vector<Description> s_available;
		
		inline explicit RegionDataBuffer(const Description& description)
		    : m_desc(description) { }
		
		Description m_desc;
	};
}
