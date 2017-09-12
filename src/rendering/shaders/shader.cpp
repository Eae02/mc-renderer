﻿#include "shader.h"
#include "shadermodules.h"
#include "../../vulkan/setlayoutsmanager.h"
#include <array>

namespace MCR
{
	Shader::Shader(VkRenderPass renderPass, const CreateInfo& createInfo)
	{
		std::vector<VkDescriptorSetLayout> dsLayouts(createInfo.setLayoutNames.size());
		std::transform(MAKE_RANGE(createInfo.setLayoutNames), dsLayouts.begin(), &GetDescriptorSetLayout);
		
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = 
		{
			/* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			/* pNext                  */ nullptr,
			/* flags                  */ 0,
			/* setLayoutCount         */ gsl::narrow<uint32_t>(dsLayouts.size()),
			/* pSetLayouts            */ dsLayouts.data(),
			/* pushConstantRangeCount */ gsl::narrow<uint32_t>(createInfo.pushConstantRanges.size()),
			/* pPushConstantRanges    */ createInfo.pushConstantRanges.data()
		};
		
		CheckResult(vkCreatePipelineLayout(vulkan.device, &pipelineLayoutCreateInfo, nullptr,
		                                   m_pipelineLayout.GetCreateAddress()));
		
		VkPipelineShaderStageCreateInfo stageCreateInfos[2];
		
		InitShaderStageCreateInfo(stageCreateInfos[0], VK_SHADER_STAGE_VERTEX_BIT, GetShaderModule(createInfo.vsName));
		
		if (!createInfo.fsName.empty())
		{
			InitShaderStageCreateInfo(stageCreateInfos[1], VK_SHADER_STAGE_FRAGMENT_BIT,
			                          GetShaderModule(createInfo.fsName));
		}
		
		const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = 
		{
			/* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			/* pNext                  */ nullptr,
			/* flags                  */ 0,
			/* topology               */ createInfo.topology,
			/* primitiveRestartEnable */ VK_FALSE
		};
		
		const VkPipelineViewportStateCreateInfo viewportState = 
		{
			/* sType         */ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			/* pNext         */ nullptr,
			/* flags         */ 0,
			/* viewportCount */ 1,
			/* pViewports    */ &createInfo.viewport,
			/* scissorCount  */ 1,
			/* pScissors     */ &createInfo.scissor
		};
		
		const VkPipelineRasterizationStateCreateInfo rasterizationState = 
		{
			/* sType                   */ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			/* pNext                   */ nullptr,
			/* flags                   */ 0,
			/* depthClampEnable        */ createInfo.enableDepthClamp,
			/* rasterizerDiscardEnable */ VK_FALSE,
			/* polygonMode             */ VK_POLYGON_MODE_FILL,
			/* cullMode                */ createInfo.cullMode,
			/* frontFace               */ createInfo.frontFace,
			/* depthBiasEnable         */ VK_FALSE,
			/* depthBiasConstantFactor */ 0,
			/* depthBiasClamp          */ 0,
			/* depthBiasSlopeFactor    */ 0,
			/* lineWidth               */ 1.0f
		};
		
		const VkPipelineMultisampleStateCreateInfo multisampleState = 
		{
			/* sType                 */ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			/* pNext                 */ nullptr,
			/* flags                 */ 0,
			/* rasterizationSamples  */ VK_SAMPLE_COUNT_1_BIT,
			/* sampleShadingEnable   */ VK_FALSE,
			/* minSampleShading      */ 0.0f,
			/* pSampleMask           */ 0,
			/* alphaToCoverageEnable */ VK_FALSE,
			/* alphaToOneEnable      */ VK_FALSE
		};
		
		const VkPipelineDepthStencilStateCreateInfo depthStencilState = 
		{
			/* sType                 */ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			/* pNext                 */ nullptr,
			/* flags                 */ 0,
			/* depthTestEnable       */ createInfo.enableDepthTest,
			/* depthWriteEnable      */ createInfo.enableDepthWrite,
			/* depthCompareOp        */ createInfo.depthCompareOp,
			/* depthBoundsTestEnable */ VK_FALSE,
			/* stencilTestEnable     */ VK_FALSE,
			/* front                 */ { },
			/* back                  */ { },
			/* minDepthBounds        */ 0,
			/* maxDepthBounds        */ 0
		};
		
		const VkPipelineColorBlendStateCreateInfo colorBlendState = 
		{
			/* sType           */ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			/* pNext           */ nullptr,
			/* flags           */ 0,
			/* logicOpEnable   */ VK_FALSE,
			/* logicOp         */ VK_LOGIC_OP_CLEAR,
			/* attachmentCount */ gsl::narrow<uint32_t>(createInfo.attachmentBlendStates.size()),
			/* pAttachments    */ createInfo.attachmentBlendStates.data(),
			/* blendConstants  */ { }
		};
		
		const VkPipelineDynamicStateCreateInfo dynamicState = 
		{
			/* sType             */ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			/* pNext             */ nullptr,
			/* flags             */ 0,
			/* dynamicStateCount */ gsl::narrow<uint32_t>(createInfo.dynamicState.size()),
			/* pDynamicStates    */ createInfo.dynamicState.data()
		};
		
		const VkGraphicsPipelineCreateInfo pipelineCreateInfo =
		{
			/* sType               */ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			/* pNext               */ nullptr,
			/* flags               */ 0,
			/* stageCount          */ createInfo.fsName.empty() ? 1u : 2u,
			/* pStages             */ stageCreateInfos,
			/* pVertexInputState   */ createInfo.vertexInputState,
			/* pInputAssemblyState */ &inputAssemblyState,
			/* pTessellationState  */ nullptr,
			/* pViewportState      */ &viewportState,
			/* pRasterizationState */ &rasterizationState,
			/* pMultisampleState   */ &multisampleState,
			/* pDepthStencilState  */ &depthStencilState,
			/* pColorBlendState    */ &colorBlendState,
			/* pDynamicState       */ &dynamicState,
			/* layout              */ *m_pipelineLayout,
			/* renderPass          */ renderPass,
			/* subpass             */ 0,
			/* basePipelineHandle  */ VK_NULL_HANDLE,
			/* basePipelineIndex   */ -1
		};
		
		CheckResult(vkCreateGraphicsPipelines(vulkan.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
		                                      nullptr, m_pipeline.GetCreateAddress()));
	}
}