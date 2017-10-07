#pragma once

#include <vulkan/vulkan.h>

#include "samplers.h"
#include "vkhandle.h"

#include <string>
#include <string_view>
#include <gsl/span>

namespace MCR
{
	struct DSLayoutBinding
	{
		VkDescriptorType m_descriptorType;
		VkShaderStageFlags m_stageFlags;
		uint32_t m_descriptorCount;
		const Samplers* m_immutableSamplers;
		
		inline DSLayoutBinding(VkDescriptorType descriptorType, VkShaderStageFlags stageFlags,
		                       uint32_t count = 1, const Samplers* immutableSamplers = nullptr)
		    : m_descriptorType(descriptorType), m_stageFlags(stageFlags), m_descriptorCount(count),
		      m_immutableSamplers(immutableSamplers) { }
	};
	
	void RegisterDescriptorSetLayout(std::string name, gsl::span<const DSLayoutBinding> bindings);
	
	VkDescriptorSetLayout GetDescriptorSetLayout(std::string_view name) noexcept;
	
	//Creates a descriptor pool which can allocate 'maxSets' descriptor sets of the given layout.
	VkHandle<VkDescriptorPool> CreateDescriptorPool(std::string_view layoutName, uint32_t maxSets);
	
	void DestroyDescriptorSetLayouts();
}
