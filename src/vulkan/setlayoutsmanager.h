#pragma once

#include <vulkan/vulkan.h>

#include "vkhandle.h"

#include <string>
#include <string_view>
#include <gsl/span>

namespace MCR
{
	struct DSLayoutBinding
	{
		VkDescriptorType   descriptorType;
		uint32_t           descriptorCount;
		VkShaderStageFlags stageFlags;
	};
	
	void RegisterDescriptorSetLayout(std::string name, gsl::span<const DSLayoutBinding> bindings);
	
	VkDescriptorSetLayout GetDescriptorSetLayout(std::string_view name) noexcept;
	
	//Creates a descriptor pool which can allocate 'maxSets' descriptor sets of the given layout.
	VkHandle<VkDescriptorPool> CreateDescriptorPool(std::string_view layoutName, uint32_t maxSets);
	
	void DestroyDescriptorSetLayouts();
}
