#pragma once

#include "shader.h"

namespace MCR
{
	class SkyShader : public Shader
	{
	public:
		SkyShader(VkRenderPass renderPass, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		void Bind(CommandBuffer& commandBuffer);
		
	private:
		static const Shader::CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
