#pragma once

#include "../vulkan/vk.h"

namespace MCR
{
	class CausticsTexture
	{
	public:
		CausticsTexture();
		
		void Render(CommandBuffer& commandBuffer);
		
		inline VkImageView GetImageView() const
		{
			return *m_imageView;
		}
		
		static void CreatePipelines();
		
		inline static void DestroyPipelines()
		{
			s_genPipeline.Reset();
			s_genPipelineLayout.Reset();
		}
		
	private:
		static VkHandle<VkPipelineLayout> s_genPipelineLayout;
		static VkHandle<VkPipeline> s_genPipeline;
		static uint32_t s_workGroupSize;
		
		UniqueDescriptorSet m_generateDescriptorSet;
		
		VkHandle<VmaAllocation> m_allocation;
		VkHandle<VkImage> m_image;
		VkHandle<VkImageView> m_imageView;
	};
}
