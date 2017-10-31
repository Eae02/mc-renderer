#pragma once

#include "shader.h"

namespace MCR
{
	class DebugShader : public Shader
	{
	public:
		DebugShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void Bind(CommandBuffer& cb, BindModes mode) const;
		
		void SetColor(CommandBuffer& cb, const glm::vec4& color) const;
		
	private:
		static const Shader::CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_globalDescriptorSet;
	};
}
