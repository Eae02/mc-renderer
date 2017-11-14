#pragma once

#include "shader.h"

namespace MCR
{
	class PostProcessShader : public Shader
	{
	public:
		explicit PostProcessShader(RenderPassInfo renderPassInfo);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		void Bind(CommandBuffer& commandBuffer);
		
	private:
		static const CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
