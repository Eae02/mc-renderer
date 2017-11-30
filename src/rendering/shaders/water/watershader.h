#pragma once

#include "../shader.h"

namespace MCR
{
	class WaterShader : public Shader
	{
	public:
#pragma pack(push, 1)
		struct Wave
		{
			glm::vec2 m_direction;
			float m_freq;
			float m_amplitude;
			float m_speed;
			float _[3];
		};
#pragma pack(pop)
		
		WaterShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void SetCausticsTexture(const class CausticsTexture& causticsTexture);
		
		void Initialize(class LoadContext& loadContext);
		
		void Bind(CommandBuffer& cb, VkDescriptorSet shadowDescriptorSet, bool underwater, BindModes bindMode) const;
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
		static constexpr size_t WaveCount = 12;
		
		float GetWaterHeightOffset(glm::vec2 position, float t) const;
		
	private:
		static const CreateInfo s_createInfo;
		
		std::array<Wave, WaveCount> m_waves;
		
		VkHandle<VmaAllocation> m_normalMapTransformsAllocation;
		VkHandle<VkBuffer> m_dataUniformBuffer;
		
		VkHandle<VmaAllocation> m_normalMapAllocation;
		VkHandle<VkImage> m_normalMap;
		VkHandle<VkImageView> m_normalMapView;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
