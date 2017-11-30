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
			const VkSpecializationInfo* tcsSpecInfo;
			const VkSpecializationInfo* tesSpecInfo;
			const VkSpecializationInfo* gsSpecInfo;
			const VkSpecializationInfo* fsSpecInfo;
		};
		
		struct CreateInfo
		{
			std::string_view                                     vsName;
			std::string_view                                     tcsName;
			std::string_view                                     tesName;
			std::string_view                                     gsName;
			std::string_view                                     fsName;
			gsl::span<const std::string_view>                    setLayoutNames;
			gsl::span<const VkPushConstantRange>                 pushConstantRanges;
			const VkPipelineVertexInputStateCreateInfo*          vertexInputState = nullptr;
			VkPrimitiveTopology                                  topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			uint32_t                                             patchControlPoints = 0;
			VkViewport                                           viewport = { 0, 0, 1, 1, 0, 1 };
			VkRect2D                                             scissor = { 0, 0, 1, 1 };
			bool                                                 enableDepthClamp = false;
			VkCullModeFlags                                      cullMode = VK_CULL_MODE_NONE;
			VkFrontFace                                          frontFace = VK_FRONT_FACE_CLOCKWISE;
			bool                                                 enableDepthTest = false;
			bool                                                 enableDepthWrite = false;
			const StencilState*                                  stencilState = nullptr;
			bool                                                 hasWireframeVariant = false;
			VkCompareOp                                          depthCompareOp = VK_COMPARE_OP_LESS;
			bool                                                 enableDepthBias = false;
			float                                                depthBiasConstantFactor = 0.0f;
			float                                                depthBiasClamp = 0.0f;
			float                                                depthBiasSlopeFactor = 0.0f;
			gsl::span<const VkPipelineColorBlendAttachmentState> attachmentBlendStates;
			gsl::span<const VkDynamicState>                      dynamicState;
			gsl::span<const Specialization>                      specializations;
			
			CreateInfo() = default;
			
			inline CreateInfo& SetVertexShaderName(std::string_view t_vsName)
			{ vsName = t_vsName; return *this; }
			
			inline CreateInfo& SetTessControlShaderName(std::string_view t_tcsName)
			{ tcsName = t_tcsName; return *this; }
			
			inline CreateInfo& SetTessEvaluationShaderName(std::string_view t_tesName)
			{ tesName = t_tesName; return *this; }
			
			inline CreateInfo& SetGeometryShaderName(std::string_view t_gsName)
			{ gsName = t_gsName; return *this; }
			
			inline CreateInfo& SetFragmentShaderName(std::string_view t_fsName)
			{ fsName = t_fsName; return *this; }
			
			inline CreateInfo& SetDSLayoutNames(gsl::span<const std::string_view> t_setLayoutNames)
			{ setLayoutNames = t_setLayoutNames; return *this; }
			
			inline CreateInfo& SetPushConstantRanges(gsl::span<const VkPushConstantRange> t_pushConstantRanges)
			{ pushConstantRanges = t_pushConstantRanges; return *this; }
			
			inline CreateInfo& SetVertexInputState(const VkPipelineVertexInputStateCreateInfo* t_vertexInputState)
			{ vertexInputState = t_vertexInputState; return *this; }
			
			inline CreateInfo& SetPatchControlPoints(uint32_t t_patchControlPoints)
			{ patchControlPoints = t_patchControlPoints; return *this; }
			
			inline CreateInfo& SetTopology(VkPrimitiveTopology t_topology)
			{ topology = t_topology; return *this; }
			
			inline CreateInfo& SetViewport(VkViewport t_viewport)
			{ viewport = t_viewport; return *this; }
			
			inline CreateInfo& SetScissor(VkRect2D t_scissor)
			{ scissor = t_scissor; return *this; }
			
			inline CreateInfo& SetEnableDepthClamp(bool t_enableDepthClamp)
			{ enableDepthClamp = t_enableDepthClamp; return *this; }
			
			inline CreateInfo& SetCullMode(VkCullModeFlags t_cullMode)
			{ cullMode = t_cullMode; return *this; }
			
			inline CreateInfo& SetFrontFace(VkFrontFace t_frontFace)
			{ frontFace = t_frontFace; return *this; }
			
			inline CreateInfo& SetEnableDepthTest(bool t_enableDepthTest)
			{ enableDepthTest = t_enableDepthTest; return *this; }
			
			inline CreateInfo& SetEnableDepthWrite(bool t_enableDepthWrite)
			{ enableDepthWrite = t_enableDepthWrite; return *this; }
			
			inline CreateInfo& SetStencilState(const StencilState* t_stencilState)
			{ stencilState = t_stencilState; return *this; }
			
			inline CreateInfo& SetHasWireframeVariant(bool t_hasWireframeVariant)
			{ hasWireframeVariant = t_hasWireframeVariant; return *this; }
			
			inline CreateInfo& SetDepthCompareOp(VkCompareOp t_depthCompareOp)
			{ depthCompareOp = t_depthCompareOp; return *this; }
			
			inline CreateInfo& SetEnableDepthBias(bool t_enableDepthBias)
			{ enableDepthBias = t_enableDepthBias; return *this; }
			
			inline CreateInfo& SetDepthBiasConstantFactor(float t_depthBiasConstantFactor)
			{ depthBiasConstantFactor = t_depthBiasConstantFactor; return *this; }
			
			inline CreateInfo& SetDepthBiasClamp(float t_depthBiasClamp)
			{ depthBiasClamp = t_depthBiasClamp; return *this; }
			
			inline CreateInfo& SetDepthBiasSlopeFactor(float t_depthBiasSlopeFactor)
			{ depthBiasSlopeFactor = t_depthBiasSlopeFactor; return *this; }
			
			inline CreateInfo& SetAttachmentBlendStates(gsl::span<const VkPipelineColorBlendAttachmentState> t_attachmentBlendStates)
			{ attachmentBlendStates = t_attachmentBlendStates; return *this; }
			
			inline CreateInfo& SetDynamicState(gsl::span<const VkDynamicState> t_dynamicState)
			{ dynamicState = t_dynamicState; return *this; }
			
			inline CreateInfo& SetSpecializations(gsl::span<const Specialization> t_specializations)
			{ specializations = t_specializations; return *this; }
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
