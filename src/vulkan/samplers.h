#pragma once

#include <vulkan/vulkan.h>

namespace MCR
{
	enum class Samplers
	{
		Framebuffer,
		ShadowMap
	};
	
	void CreateSamplers();
	void DestroySamplers();
	
	VkSampler GetSampler(Samplers sampler);
}
