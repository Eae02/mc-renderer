#include "samplers.h"
#include "destroy.h"

namespace MCR
{
	static const VkSamplerCreateInfo framebufferSamplerCreateInfo = 
	{
		/* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		/* pNext                   */ nullptr,
		/* flags                   */ 0,
		/* magFilter               */ VK_FILTER_NEAREST,
		/* minFilter               */ VK_FILTER_NEAREST,
		/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
		/* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* mipLodBias              */ 0.0f,
		/* anisotropyEnable        */ VK_FALSE,
		/* maxAnisotropy           */ 0,
		/* compareEnable           */ VK_FALSE,
		/* compareOp               */ VK_COMPARE_OP_ALWAYS,
		/* minLod                  */ 0.0f,
		/* maxLod                  */ 0,
		/* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		/* unnormalizedCoordinates */ VK_FALSE
	};
	
	static const VkSamplerCreateInfo framebufferDownsampledSamplerCreateInfo = 
	{
		/* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		/* pNext                   */ nullptr,
		/* flags                   */ 0,
		/* magFilter               */ VK_FILTER_LINEAR,
		/* minFilter               */ VK_FILTER_LINEAR,
		/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
		/* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* mipLodBias              */ 0.0f,
		/* anisotropyEnable        */ VK_FALSE,
		/* maxAnisotropy           */ 0,
		/* compareEnable           */ VK_FALSE,
		/* compareOp               */ VK_COMPARE_OP_ALWAYS,
		/* minLod                  */ 0.0f,
		/* maxLod                  */ 0,
		/* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		/* unnormalizedCoordinates */ VK_FALSE
	};
	
	static const VkSamplerCreateInfo windNoiseSamplerCreateInfo = 
	{
		/* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		/* pNext                   */ nullptr,
		/* flags                   */ 0,
		/* magFilter               */ VK_FILTER_LINEAR,
		/* minFilter               */ VK_FILTER_LINEAR,
		/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
		/* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
		/* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
		/* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_REPEAT,
		/* mipLodBias              */ 0.0f,
		/* anisotropyEnable        */ VK_FALSE,
		/* maxAnisotropy           */ 0,
		/* compareEnable           */ VK_FALSE,
		/* compareOp               */ VK_COMPARE_OP_ALWAYS,
		/* minLod                  */ 0.0f,
		/* maxLod                  */ 0,
		/* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		/* unnormalizedCoordinates */ VK_FALSE
	};
	
	static const VkSamplerCreateInfo shadowMapSamplerCreateInfo = 
	{
		/* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		/* pNext                   */ nullptr,
		/* flags                   */ 0,
		/* magFilter               */ VK_FILTER_LINEAR,
		/* minFilter               */ VK_FILTER_LINEAR,
		/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
		/* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* mipLodBias              */ 0.0f,
		/* anisotropyEnable        */ VK_FALSE,
		/* maxAnisotropy           */ 0,
		/* compareEnable           */ VK_TRUE,
		/* compareOp               */ VK_COMPARE_OP_LESS,
		/* minLod                  */ 0.0f,
		/* maxLod                  */ 0,
		/* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		/* unnormalizedCoordinates */ VK_FALSE
	};
	
	static VkSampler samplers[4] = { };
	
	template <Samplers sampler>
	inline void CreateSampler(const VkSamplerCreateInfo& createInfo)
	{
		vkCreateSampler(vulkan.device, &createInfo, nullptr, &samplers[static_cast<int>(sampler)]);
	}
	
	void CreateSamplers()
	{
		CreateSampler<Samplers::Framebuffer>(framebufferSamplerCreateInfo);
		CreateSampler<Samplers::FramebufferDownsampled>(framebufferDownsampledSamplerCreateInfo);
		CreateSampler<Samplers::WindNoise>(windNoiseSamplerCreateInfo);
		CreateSampler<Samplers::ShadowMap>(shadowMapSamplerCreateInfo);
	}
	
	void DestroySamplers()
	{
		for (VkSampler& sampler : samplers)
		{
			DestroyVulkanObject(sampler);
			sampler = VK_NULL_HANDLE;
		}
	}
	
	VkSampler GetSampler(Samplers sampler)
	{
		return samplers[static_cast<int>(sampler)];
	}
}
