#pragma once

#include "shader.h"

namespace MCR
{
	class WaterShader : public Shader
	{
	public:
		WaterShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo);
		
		void Initialize(class LoadContext& loadContext);
		
		void Bind(CommandBuffer& cb) const;
		
		void FramebufferChanged(const class Framebuffer& framebuffer);
		
	private:
		static const CreateInfo s_createInfo;
		
		VkHandle<VmaAllocation> m_normalMapTransformsAllocation;
		VkHandle<VkBuffer> m_normalMapTransformsBuffer;
		
		VkHandle<VmaAllocation> m_normalMapAllocation;
		VkHandle<VkImage> m_normalMap;
		VkHandle<VkImageView> m_normalMapView;
		
		UniqueDescriptorSet m_descriptorSet;
	};
}
