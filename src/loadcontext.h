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
		struct GenericResource
		{
			virtual ~GenericResource() { }
		};
		
		template <typename ResourceTp>
		struct Resource : public GenericResource
		{
			inline explicit Resource(ResourceTp resource)
			    : m_resource(std::move(resource)) { }
			
			ResourceTp m_resource;
		};
		
	public:
		LoadContext();
		
		template <typename ResourceTp>
		inline void TakeResource(ResourceTp&& resource)
		{
			m_resources.push_back(std::make_unique<Resource<std::decay_t<ResourceTp>>>(std::move(resource)));
		}
		
		inline CommandBuffer& GetCB()
		{
			return m_commandBuffer;
		}
		
		void Begin();
		
		void End();
		
	private:
		std::vector<std::unique_ptr<GenericResource>> m_resources;
		
		VkHandle<VkCommandPool> m_commandPool;
		
		CommandBuffer m_commandBuffer;
	};
}
