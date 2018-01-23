#pragma once

#include <glm/glm.hpp>
#include "../vulkan/vk.h"
#include "shaders/shader.h"

namespace MCR
{
	class StarRenderer
	{
	public:
		StarRenderer(Shader::RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void Initialize(class LoadContext& loadContext);
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		void Render(CommandBuffer& commandBuffer, const class TimeManager& timeManager);
		
	private:
		static const VkVertexInputBindingDescription s_vertexInputBindings[1];
		static const VkVertexInputAttributeDescription s_vertexInputAttributes[2];
		
		static const VkPipelineVertexInputStateCreateInfo s_vertexInputState;
		static const Shader::CreateInfo s_starShaderCreateInfo;
		
#pragma pack(push, 1)
		struct Star
		{
			glm::vec3 m_color;
			float _padding;
			glm::vec3 m_direction;
			float m_distance;
		};
#pragma pack(pop)
		
		VkHandle<VmaAllocation> m_starFieldAllocation;
		VkHandle<VkBuffer> m_starFieldBuffer;
		
		glm::vec2 m_starSize;
		
		Shader m_starShader;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
