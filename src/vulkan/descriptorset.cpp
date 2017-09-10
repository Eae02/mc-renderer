#include "descriptorset.h"
#include "setlayoutsmanager.h"
#include "vkutils.h"

namespace MCR
{
	UniqueDescriptorSet::UniqueDescriptorSet(std::string_view layoutName)
	    : m_pool(CreateDescriptorPool(layoutName, 1))
	{
		VkDescriptorSetLayout layout = GetDescriptorSetLayout(layoutName);
		
		const VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = 
		{
			/* sType              */ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			/* pNext              */ nullptr,
			/* descriptorPool     */ *m_pool,
			/* descriptorSetCount */ 1,
			/* pSetLayouts        */ &layout
		};
		
		CheckResult(vkAllocateDescriptorSets(vulkan.device, &descriptorSetAllocateInfo, &m_descriptorSet));
	}
}
