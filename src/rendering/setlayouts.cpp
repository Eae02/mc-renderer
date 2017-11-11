#include "setlayouts.h"
#include "../vulkan/setlayoutsmanager.h"

namespace MCR
{
	const Samplers windNoiseSamplers[] = { Samplers::WindNoise };
	
	static DSLayoutBinding BlockShader_Global[] = 
	{
		//Render settings buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Blocks texture array
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Wind noise texture
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT, 1, windNoiseSamplers }
	};
	
	const Samplers shadowMapSamplers[] = { Samplers::ShadowMap };
	
	static DSLayoutBinding ShadowSample[] = 
	{
		//Shadow map
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, shadowMapSamplers },
		
		//Shadow info buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Wind noise texture
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT, 1, windNoiseSamplers }
	};
	
	static DSLayoutBinding BlockShaderShadow_Global[] = 
	{
		//Render settings buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		
		//Light matrices buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_GEOMETRY_BIT },
		
		//Blocks texture array
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Wind noise texture
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_VERTEX_BIT, 1, windNoiseSamplers }
	};
	
	static DSLayoutBinding DebugShader_Global[] = 
	{
		//Render settings buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }
	};
	
	const Samplers framebufferSamplers[] = { Samplers::Framebuffer };
	const Samplers framebufferSamplersDS[] = { Samplers::FramebufferDownsampled };
	
	static DSLayoutBinding Sky[] = 
	{
		//Render settings buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Color image
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, framebufferSamplers },
		
		//Depth image
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, framebufferSamplers },
		
		//God rays image
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, framebufferSamplers },
	};
	
	static DSLayoutBinding GodRaysGen[] = 
	{
		//Render settings buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Depth image
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, framebufferSamplers }
	};
	
	static DSLayoutBinding GodRaysBlur[] = 
	{
		//God rays image
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, framebufferSamplersDS }
	};
	
	const Samplers waterNormalsSamplers[] = { Samplers::WaterNormalMap };
	
	static DSLayoutBinding Water[] =
	{
		//Render settings buffer
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
		
		//Color image
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, framebufferSamplers },
		
		//Depth image
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, framebufferSamplers },
		
		//Normal map transforms
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT },
		
		//Normal map
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1, waterNormalsSamplers }
	};
	
	static DSLayoutBinding UI_Sampler[] = 
	{
		//Sampler
		{ VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
	};
	
	static DSLayoutBinding UI_Image[] = 
	{
		//Image
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_FRAGMENT_BIT }
	};
	
	void RegisterSetLayouts()
	{
		RegisterDescriptorSetLayout("BlockShader_Global", BlockShader_Global);
		RegisterDescriptorSetLayout("ShadowSample", ShadowSample);
		RegisterDescriptorSetLayout("BlockShaderShadow_Global", BlockShaderShadow_Global);
		RegisterDescriptorSetLayout("DebugShader_Global", DebugShader_Global);
		RegisterDescriptorSetLayout("Sky", Sky);
		RegisterDescriptorSetLayout("GodRaysGen", GodRaysGen);
		RegisterDescriptorSetLayout("GodRaysBlur", GodRaysBlur);
		RegisterDescriptorSetLayout("Water", Water);
		RegisterDescriptorSetLayout("UI_Sampler", UI_Sampler);
		RegisterDescriptorSetLayout("UI_Image", UI_Image);
	}
}
