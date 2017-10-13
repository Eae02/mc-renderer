#pragma once

#include "vulkan/vk.h"

#include <vector>
#include <memory>
#include <utility>
#include <type_traits>

namespace MCR
{
	class LoadContext
	{
		static constexpr size_t ResourceSize = 8;
		
	public:
		LoadContext();
		
		template <typename ResourceTp>
		inline void TakeResource(VkHandle<ResourceTp> resource)
		{
			static_assert(sizeof(ResourceTp) <= ResourceSize, "LoadContext resource type is too large.");
			
			Resource& resourceEntry = m_resources.emplace_back();
			
			*reinterpret_cast<ResourceTp*>(resourceEntry.m_resource) = resource.Release();
			resourceEntry.m_destroy = [] (const void* res)
			{
				DestroyVulkanObject(*reinterpret_cast<const ResourceTp*>(res));
			};
		}
		
		inline CommandBuffer& GetCB()
		{
			return m_commandBuffer;
		}
		
		void Begin();
		
		void End();
		
	private:
		struct Resource
		{
			void(*m_destroy)(const void*);
			std::byte m_resource[ResourceSize];
		};
		
		std::vector<Resource> m_resources;
		
		VkHandle<VkCommandPool> m_commandPool;
		
		CommandBuffer m_commandBuffer;
	};
}
