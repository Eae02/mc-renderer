#pragma once

#include <vulkan/vulkan.h>

namespace MCR
{
	enum class Samplers
	{
		Framebuffer = 0,
		FramebufferDownsampled = 1,
		WindNoise = 2,
		WaterNormalMap = 2, //Uses the same settings as the wind noise sampler.
		ShadowMap = 3
	};
	
	void CreateSamplers();
	void DestroySamplers();
	
	VkSampler GetSampler(Samplers sampler);
}
