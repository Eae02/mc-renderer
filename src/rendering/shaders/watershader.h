#pragma once

#include "shader.h"

namespace MCR
{
	class WaterShader : public Shader
	{
	public:
		WaterShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void Bind(CommandBuffer& cb) const;
		
	private:
		static const CreateInfo s_createInfo;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
