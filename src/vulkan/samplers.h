#pragma once

#include <vulkan/vulkan.h>

namespace MCR
{
	enum class Samplers
	{
		Framebuffer,
		WindNoise,
		ShadowMap
	};
	
	void CreateSamplers();
	void DestroySamplers();
	
	VkSampler GetSampler(Samplers sampler);
}
