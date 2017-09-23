#pragma once

#include <vector>

#include "func.h"
#include "instance.h"

namespace MCR
{
	template <typename T>
	inline void DestroyVulkanObject(T object);
	
#define DEF_DESTROY_VULKAN_OBJECT(type) \
	template <> inline void DestroyVulkanObject(Vk ## type object) \
	{ \
		vkDestroy ## type(vulkan.device, object, nullptr); \
	}
	
	template <> inline void DestroyVulkanObject(VmaAllocation allocation)
	{
		vmaFreeMemory(vulkan.allocator, allocation);
	}
	
	DEF_DESTROY_VULKAN_OBJECT(Image)
	DEF_DESTROY_VULKAN_OBJECT(ImageView)
	DEF_DESTROY_VULKAN_OBJECT(Framebuffer)
	DEF_DESTROY_VULKAN_OBJECT(Sampler)
	DEF_DESTROY_VULKAN_OBJECT(Semaphore)
	DEF_DESTROY_VULKAN_OBJECT(Fence)
	DEF_DESTROY_VULKAN_OBJECT(CommandPool)
	DEF_DESTROY_VULKAN_OBJECT(DescriptorPool)
	DEF_DESTROY_VULKAN_OBJECT(DescriptorSetLayout)
	DEF_DESTROY_VULKAN_OBJECT(Pipeline)
	DEF_DESTROY_VULKAN_OBJECT(PipelineLayout)
	DEF_DESTROY_VULKAN_OBJECT(RenderPass)
	DEF_DESTROY_VULKAN_OBJECT(Buffer)
	DEF_DESTROY_VULKAN_OBJECT(QueryPool)
	DEF_DESTROY_VULKAN_OBJECT(ShaderModule)
	DEF_DESTROY_VULKAN_OBJECT(SwapchainKHR)
	
#undef DEF_DESTROY_VULKAN_OBJECT
	
	namespace Detail
	{
		void AddToDestroyList(void (*destroyCallback)(void*), void* object);
	}
	
	template <typename T>
	inline void DelayedDestroyVulkanObject(T object)
	{
		Detail::AddToDestroyList([] (void* objectVP)
		{
			DestroyVulkanObject<T>(reinterpret_cast<T>(objectVP));
		}, reinterpret_cast<void*>(object));
	}
	
	void ProcessVulkanDestroyList();
	
	void ClearVulkanDestroyList();
}
