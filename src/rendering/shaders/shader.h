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
			std::string_view                                     gsName;
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
			bool                                                 hasWireframeVariant;
			VkCompareOp                                          depthCompareOp;
			bool                                                 enableDepthBias;
			float                                                depthBiasConstantFactor;
			float                                                depthBiasClamp;
			float                                                depthBiasSlopeFactor;
			gsl::span<const VkPipelineColorBlendAttachmentState> attachmentBlendStates;
			gsl::span<const VkDynamicState>                      dynamicState;
		};
		
		struct RenderPassInfo
		{
			VkRenderPass m_renderPass;
			uint32_t m_subpass;
		};
		
		enum class BindModes
		{
			Default,
			Wireframe
		};
		
		void Bind(CommandBuffer& cb, BindModes mode = BindModes::Default) const;
		
		inline VkPipelineLayout GetLayout() const
		{
			return *m_pipelineLayout;
		}
		
		Shader(RenderPassInfo renderPassInfo, const CreateInfo& createInfo);
		
	private:
		VkHandle<VkPipelineLayout> m_pipelineLayout;
		VkHandle<VkPipeline> m_pipeline;
		VkHandle<VkPipeline> m_wireframePipeline;
	};
}
