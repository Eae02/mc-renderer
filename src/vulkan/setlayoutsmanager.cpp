#include "setlayoutsmanager.h"
#include "vkutils.h"

#include <array>
#include <vector>
#include <algorithm>

namespace MCR
{
	static constexpr uint32_t NumDescriptorTypes = 10;
	
	struct DSLayout
	{
		std::string m_name;
		VkHandle<VkDescriptorSetLayout> m_layout;
		std::array<uint32_t, NumDescriptorTypes> m_descriptorCounts;
		
		DSLayout() = default;
		
		inline DSLayout(std::string name, VkHandle<VkDescriptorSetLayout> layout)
		    : m_name(std::move(name)), m_layout(std::move(layout))
		{
			std::fill(MAKE_RANGE(m_descriptorCounts), 0);
		}
	};
	
	static std::vector<DSLayout> layouts;
	
	void RegisterDescriptorSetLayout(std::string name, gsl::span<const DSLayoutBinding> bindings)
	{
		//Counts the number of immutable samplers.
		uint32_t numImmutableSamplers = 0;
		for (const DSLayoutBinding& layoutBinding : bindings)
		{
			if (layoutBinding.m_immutableSamplers != nullptr)
			{
				numImmutableSamplers += layoutBinding.m_descriptorCount;
			}
		}
		
		VkSampler* nextImmutableSampler = nullptr;
		if (numImmutableSamplers > 0)
		{
			void* immutableSamplersMem = alloca(numImmutableSamplers * sizeof(VkSampler));
			nextImmutableSampler = reinterpret_cast<VkSampler*>(immutableSamplersMem);
		}
		
		void* vkBindingsMem = alloca(bindings.size() * sizeof(VkDescriptorSetLayoutBinding));
		VkDescriptorSetLayoutBinding* vkBindings = reinterpret_cast<VkDescriptorSetLayoutBinding*>(vkBindingsMem);
		
		const uint32_t bindingCount = gsl::narrow<uint32_t>(bindings.size());
		
		for (uint32_t i = 0; i < bindingCount; i++)
		{
			vkBindings[i].binding = i;
			vkBindings[i].descriptorType = bindings[i].m_descriptorType;
			vkBindings[i].descriptorCount = bindings[i].m_descriptorCount;
			vkBindings[i].stageFlags = bindings[i].m_stageFlags;
			
			if (bindings[i].m_immutableSamplers != nullptr)
			{
				vkBindings[i].pImmutableSamplers = nextImmutableSampler;
				
				//Copies immutable samplers.
				for (uint32_t j = 0; j < bindings[i].m_descriptorCount; j++)
				{
					*(nextImmutableSampler++) = GetSampler(bindings[i].m_immutableSamplers[j]);
				}
			}
			else
			{
				vkBindings[i].pImmutableSamplers = nullptr;;
			}
		}
		
		const VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
		{
			/* sType        */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			/* pNext        */ nullptr,
			/* flags        */ 0,
			/* bindingCount */ bindingCount,
			/* pBindings    */ vkBindings
		};
		
		VkDescriptorSetLayout layout;
		CheckResult(vkCreateDescriptorSetLayout(vulkan.device, &layoutCreateInfo, nullptr, &layout));
		
		layouts.emplace_back(std::move(name), layout);
		
		for (const DSLayoutBinding& binding : bindings)
		{
			layouts.back().m_descriptorCounts[static_cast<int>(binding.m_descriptorType)] += binding.m_descriptorCount;
		}
	}
	
	const DSLayout& FindSetLayout(std::string_view name)
	{
		auto layoutIt = std::find_if(MAKE_RANGE(layouts), [&] (const DSLayout& layout)
		{
			return layout.m_name == name;
		});
		
		if (layoutIt == layouts.end())
		{
			Log("Unknown descriptor set layout '", name, "'.");
			std::terminate();
		}
		
		return *layoutIt;
	}
	
	VkDescriptorSetLayout GetDescriptorSetLayout(std::string_view name) noexcept
	{
		return *FindSetLayout(name).m_layout;
	}
	
	void DestroyDescriptorSetLayouts()
	{
		layouts.clear();
	}
	
	VkHandle<VkDescriptorPool> CreateDescriptorPool(std::string_view layoutName, uint32_t maxSets)
	{
		const DSLayout& layout = FindSetLayout(layoutName);
		
		std::array<VkDescriptorPoolSize, NumDescriptorTypes> poolSizes;
		uint32_t numPoolSizes = 0;
		
		for (uint32_t i = 0; i < NumDescriptorTypes; i++)
		{
			if (layout.m_descriptorCounts[i] == 0)
				continue;
			
			poolSizes[numPoolSizes++] = { static_cast<VkDescriptorType>(i), layout.m_descriptorCounts[i] * maxSets };
		}
		
		const VkDescriptorPoolCreateInfo createInfo = 
		{
			/* sType         */ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			/* pNext         */ nullptr,
			/* flags         */ VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			/* maxSets       */ maxSets,
			/* poolSizeCount */ numPoolSizes,
			/* pPoolSizes    */ poolSizes.data()
		};
		
		VkDescriptorPool descriptorPool;
		CheckResult(vkCreateDescriptorPool(vulkan.device, &createInfo, nullptr, &descriptorPool));
		
		return descriptorPool;
	}
}
