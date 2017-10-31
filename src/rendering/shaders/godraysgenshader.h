#pragma once

#include "shader.h"

namespace MCR
{
	class GodRaysGenShader : public Shader
	{
	public:
		explicit GodRaysGenShader(RenderPassInfo renderPass, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		void Bind(CommandBuffer& commandBuffer);
		
	private:
		static const CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
