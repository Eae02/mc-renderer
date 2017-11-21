#pragma once

#include "../../vulkan/vk.h"

#include <vector>

namespace MCR
{
	class Shader
	{
	public:
		struct StencilState
		{
			VkStencilOpState front;
			VkStencilOpState back;
		};
		
		struct Specialization
		{
			const VkSpecializationInfo* vsSpecInfo;
			const VkSpecializationInfo* gsSpecInfo;
			const VkSpecializationInfo* fsSpecInfo;
		};
		
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
			const StencilState*                                  stencilState;
			bool                                                 hasWireframeVariant;
			VkCompareOp                                          depthCompareOp;
			bool                                                 enableDepthBias;
			float                                                depthBiasConstantFactor;
			float                                                depthBiasClamp;
			float                                                depthBiasSlopeFactor;
			gsl::span<const VkPipelineColorBlendAttachmentState> attachmentBlendStates;
			gsl::span<const VkDynamicState>                      dynamicState;
			gsl::span<const Specialization>                      specializations;
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
		
		void Bind(CommandBuffer& cb, BindModes mode = BindModes::Default, uint32_t permutation = 0) const;
		
		inline VkPipelineLayout GetLayout() const
		{
			return *m_pipelineLayout;
		}
		
		Shader(RenderPassInfo renderPassInfo, const CreateInfo& createInfo);
		
		static void InitializeCache(const fs::path& cachePath);
		static void SaveCache(const fs::path& cachePath);
		static void DestroyCache();
		
		inline static VkPipelineCache GetCache()
		{
			return s_pipelineCache;
		}
		
	private:
		static VkPipelineCache s_pipelineCache;
		
		VkHandle<VkPipelineLayout> m_pipelineLayout;
		std::vector<VkHandle<VkPipeline>> m_pipelines;
		std::vector<VkHandle<VkPipeline>> m_wireframePipelines;
	};
}
