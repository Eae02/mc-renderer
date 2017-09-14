#pragma once

#include "shader.h"

namespace MCR
{
	class BlockShader : public Shader
	{
	public:
		BlockShader(VkRenderPass renderPass, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void Bind(CommandBuffer& cb, BindModes mode) const;
		
	private:
		static const Shader::CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_globalDescriptorSet;
	};
}
