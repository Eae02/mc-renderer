#pragma once

#include "../shader.h"

namespace MCR
{
	class WaterPostShader : public Shader
	{
	public:
		WaterPostShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void SetCausticsTexture(const class CausticsTexture& causticsTexture);
		
		void Bind(CommandBuffer& cb, VkDescriptorSet shadowDescriptorSet, bool underwater, float waterPlaneY) const;
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
	private:
		static const CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
