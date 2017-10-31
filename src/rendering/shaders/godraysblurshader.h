#pragma once

#include "shader.h"

namespace MCR
{
	class GodRaysBlurShader : public Shader
	{
	public:
		explicit GodRaysBlurShader(RenderPassInfo renderPass);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		void Bind(CommandBuffer& commandBuffer);
		
	private:
		static const CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
