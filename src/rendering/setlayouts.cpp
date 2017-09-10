#include "setlayouts.h"
#include "../vulkan/setlayoutsmanager.h"

namespace MCR
{
	static DSLayoutBinding BlockShader_Global[] = 
	{
		//Render settings buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Blocks texture array
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
	};
	
	void RegisterSetLayouts()
	{
		RegisterDescriptorSetLayout("BlockShader_Global", BlockShader_Global);
	}
}
