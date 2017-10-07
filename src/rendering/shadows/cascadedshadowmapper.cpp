#include "cascadedshadowmapper.h"
#include "../vertex.h"

namespace MCR
{
	constexpr uint32_t CascadedShadowMapper::NumCascades;
	
	static VkRenderPass CreateRenderPass()
	{
		const VkAttachmentDescription attachmentDescription = 
		{
			/* flags          */ 0,
			/* format         */ vulkan.depthFormat,
			/* samples        */ VK_SAMPLE_COUNT_1_BIT,
			/* loadOp         */ VK_ATTACHMENT_LOAD_OP_CLEAR,
			/* storeOp        */ VK_ATTACHMENT_STORE_OP_STORE,
			/* stencilLoadOp  */ VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			/* stencilStoreOp */ VK_ATTACHMENT_STORE_OP_DONT_CARE,
			/* initialLayout  */ VK_IMAGE_LAYOUT_UNDEFINED,
			/* finalLayout    */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};
		
		const VkAttachmentReference attachmentRef = { 0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		
		VkSubpassDescription subpassDescription = { };
		subpassDescription.pDepthStencilAttachment = &attachmentRef;
		
		std::array<VkSubpassDependency, 2> subpassDependencies;
		
		subpassDependencies[0] = 
		{
			/* srcSubpass      */ VK_SUBPASS_EXTERNAL,
			/* dstSubpass      */ 0,
			/* srcStageMask    */ VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			/* dstStageMask    */ VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			/* srcAccessMask   */ VK_ACCESS_SHADER_READ_BIT,
			/* dstAccessMask   */ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			/* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
		};
		
		subpassDependencies[1] = 
		{
			/* srcSubpass      */ 0,
			/* dstSubpass      */ VK_SUBPASS_EXTERNAL,
			/* srcStageMask    */ VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			/* dstStageMask    */ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			/* srcAccessMask   */ VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			/* dstAccessMask   */ VK_ACCESS_SHADER_READ_BIT,
			/* dependencyFlags */ VK_DEPENDENCY_BY_REGION_BIT
		};
		
		const VkRenderPassCreateInfo renderPassCreateInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			/* pNext           */ nullptr,
			/* flags           */ 0,
			/* attachmentCount */ 1,
			/* pAttachments    */ &attachmentDescription,
			/* subpassCount    */ 1,
			/* pSubpasses      */ &subpassDescription,
			/* dependencyCount */ static_cast<uint32_t>(subpassDependencies.size()),
			/* pDependencies   */ subpassDependencies.data()
		};
		
		VkRenderPass renderPass;
		CheckResult(vkCreateRenderPass(vulkan.device, &renderPassCreateInfo, nullptr, &renderPass));
		return renderPass;
	}
	
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "BlockShaderShadow_Global" };
	
	static const Shader::CreateInfo shaderCreateInfo = 
	{
		/* vsName                */ "block-shadow.vs",
		/* gsName                */ "block-shadow.gs",
		/* fsName                */ "block-shadow.fs",
		/* setLayoutNames        */ setLayouts,
		/* pushConstantRanges    */ { },
		/* vertexInputState      */ &blockVertexShadowInputState,
		/* topology              */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport              */ { 0, 0, 1, 1, 0, 1 },
		/* scissor               */ { 0, 0, 1, 1 },
		/* enableDepthClamp      */ false,
		/* cullMode              */ VK_CULL_MODE_FRONT_BIT,
		/* frontFace             */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest       */ true,
		/* enableDepthWrite      */ true,
		/* hasWireframeVariant   */ false,
		/* depthCompareOp        */ VK_COMPARE_OP_LESS,
		/* attachmentBlendStates */ { },
		/* dynamicState          */ dynamicState
	};
	
	CascadedShadowMapper::CascadedShadowMapper()
	    : m_renderPass(CreateRenderPass()), m_shader(*m_renderPass, shaderCreateInfo)
	{
		
	}
	
	void CascadedShadowMapper::SetResolution(uint32_t resolution)
	{
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_2D, vulkan.depthFormat, resolution, resolution);
		imageCreateInfo.arrayLayers = NumCascades;
		imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		CheckResult(vkCreateImage(vulkan.device, &imageCreateInfo, nullptr, m_shadowMap.GetCreateAddress()));
		
		VkImageViewCreateInfo imageViewCreateInfo;
		InitImageViewCreateInfo(imageViewCreateInfo, *m_shadowMap, VK_IMAGE_VIEW_TYPE_2D_ARRAY, vulkan.depthFormat,
		                        { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, NumCascades });
		
		CheckResult(vkCreateImageView(vulkan.device, &imageViewCreateInfo, nullptr,
		                              m_shadowMapView.GetCreateAddress()));
		
		const VkFramebufferCreateInfo framebufferCreateInfo = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			/* pNext           */ nullptr,
			/* flags           */ 0,
			/* renderPass      */ *m_renderPass,
			/* attachmentCount */ 1,
			/* pAttachments    */ &*m_shadowMapView,
			/* width           */ resolution,
			/* height          */ resolution,
			/* layers          */ NumCascades
		};
		
		CheckResult(vkCreateFramebuffer(vulkan.device, &framebufferCreateInfo, nullptr,
		                                m_framebuffer.GetCreateAddress()));
		
		m_resolution = resolution;
	}
	
	void CascadedShadowMapper::Render()
	{
		
	}
	
	
}
