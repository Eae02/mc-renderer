#pragma once

#include "../../vulkan/vk.h"

namespace MCR
{
	class Shader
	{
	public:
		struct CreateInfo
		{
			std::string_view                                     vsName;
			std::string_view                                     fsName;
			gsl::span<const std::string_view>                    setLayoutNames;
			gsl::span<const VkPushConstantRange>                 pushConstantRanges;
			const VkPipelineVertexInputStateCreateInfo*          vertexInputState;
			VkPrimitiveTopology                                  topology;
			VkViewport                                           viewport;
			VkRect2D                                             scissor;
			bool                                                 enableDepthClamp;
			VkCullModeFlags                                      cullMode;
			VkFrontFace                                          frontFace;
			bool                                                 enableDepthTest;
			bool                                                 enableDepthWrite;
			VkCompareOp                                          depthCompareOp;
			gsl::span<const VkPipelineColorBlendAttachmentState> attachmentBlendStates;
			gsl::span<const VkDynamicState>                      dynamicState;
		};
		
		inline void Bind(CommandBuffer& cb) const
		{
			cb.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		}
		
		inline VkPipelineLayout GetLayout() const
		{
			return *m_pipelineLayout;
		}
		
	protected:
		Shader(VkRenderPass renderPass, const CreateInfo& createInfo);
		
	private:
		VkHandle<VkPipelineLayout> m_pipelineLayout;
		VkHandle<VkPipeline> m_pipeline;
	};
}
