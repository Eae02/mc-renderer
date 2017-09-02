#pragma once

#include <vulkan/vulkan.h>
#include <utility>

#include "destroy.h"
#include "../utils.h"

namespace MCR
{
	enum class VkHandleDestroyTime
	{
		Instant,
		Delayed
	};
	
	//RAII wrapper for graphics resources
	template <typename VkType, VkHandleDestroyTime DestroyTime = VkHandleDestroyTime::Instant>
	class VkHandle final
	{
	public:
		inline VkHandle() : m_resource(nullptr) { }
		
		inline VkHandle(VkType resource)
			: m_resource(resource) { }
		
		inline ~VkHandle()
		{
			Reset();
		}
		
		template <VkHandleDestroyTime OtherDestroyTime>
		inline VkHandle(VkHandle<VkType, OtherDestroyTime>&& other) noexcept
			: m_resource(other.m_resource)
		{
			other.m_resource = nullptr;
		}
		
		template <VkHandleDestroyTime OtherDestroyTime>
		inline VkHandle& operator=(VkHandle<VkType, OtherDestroyTime>&& other)
		{
			this->operator=(other.m_resource);
			other.m_resource = nullptr;
			return *this;
		}
		
		inline VkHandle& operator=(VkType resource)
		{
			Reset();
			m_resource = resource;
			return *this;
		}
		
		VkHandle(const VkHandle&) = delete;
		VkHandle& operator=(const VkHandle&) = delete;
		
		inline void Reset()
		{
			if (m_resource)
			{
				constexpr_if (DestroyTime == VkHandleDestroyTime::Instant)
				{
					DestroyVulkanObject(m_resource);
				}
				else
				{
					DelayedDestroyVulkanObject(m_resource);
				}
				
				m_resource = nullptr;
			}
		}
		
		inline VkType* GetCreateAddress()
		{
			Reset();
			return &m_resource;
		}
		
		inline bool IsNull() const
		{
			return m_resource == VK_NULL_HANDLE;
		}
		
		inline void Swap(VkHandle<VkType, DestroyTime>& other)
		{
			std::swap(m_resource, other.m_resource);
		}
		
		inline const VkType& operator*() const
		{
			return m_resource;
		}
		
	private:
		VkType m_resource;
	};
	
	class VmaAllocationDeleter
	{
	public:
		inline explicit VmaAllocationDeleter(VmaAllocator allocator = vulkan.allocator)
		    : m_allocator(allocator) { }
		
		inline void operator()(VmaAllocation allocation) const
		{
			vmaFreeMemory(m_allocator, allocation);
		}
		
	private:
		VmaAllocator m_allocator;
	};
	
	using VmaAllocationHandle = std::unique_ptr<std::remove_pointer_t<VmaAllocation>, VmaAllocationDeleter>;
}
