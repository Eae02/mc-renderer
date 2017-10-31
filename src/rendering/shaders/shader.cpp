#include "shader.h"
#include "shadermodules.h"
#include "../../vulkan/setlayoutsmanager.h"
#include <array>

namespace MCR
{
	Shader::Shader(RenderPassInfo renderPassInfo, const CreateInfo& createInfo)
	{
		void* dsLayoytsMem = alloca(createInfo.setLayoutNames.size() * sizeof(VkDescriptorSetLayout));
		VkDescriptorSetLayout* dsLayouts = reinterpret_cast<VkDescriptorSetLayout*>(dsLayoytsMem);
		std::transform(MAKE_RANGE(createInfo.setLayoutNames), dsLayouts, &GetDescriptorSetLayout);
		
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = 
		{
			/* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			/* pNext                  */ nullptr,
			/* flags                  */ 0,
			/* setLayoutCount         */ gsl::narrow<uint32_t>(createInfo.setLayoutNames.size()),
			/* pSetLayouts            */ dsLayouts,
			/* pushConstantRangeCount */ gsl::narrow<uint32_t>(createInfo.pushConstantRanges.size()),
			/* pPushConstantRanges    */ createInfo.pushConstantRanges.data()
		};
		
		CheckResult(vkCreatePipelineLayout(vulkan.device, &pipelineLayoutCreateInfo, nullptr,
		                                   m_pipelineLayout.GetCreateAddress()));
		
		VkPipelineShaderStageCreateInfo stageCreateInfos[3];
		
		InitShaderStageCreateInfo(stageCreateInfos[0], VK_SHADER_STAGE_VERTEX_BIT, GetShaderModule(createInfo.vsName));
		
		uint32_t numStageCreateInfos = 1;
		
		if (!createInfo.gsName.empty())
		{
			InitShaderStageCreateInfo(stageCreateInfos[numStageCreateInfos++], VK_SHADER_STAGE_GEOMETRY_BIT,
			                          GetShaderModule(createInfo.gsName));
		}
		
		if (!createInfo.fsName.empty())
		{
			InitShaderStageCreateInfo(stageCreateInfos[numStageCreateInfos++], VK_SHADER_STAGE_FRAGMENT_BIT,
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
			/* depthBiasEnable         */ createInfo.enableDepthBias,
			/* depthBiasConstantFactor */ createInfo.depthBiasConstantFactor,
			/* depthBiasClamp          */ createInfo.depthBiasClamp,
			/* depthBiasSlopeFactor    */ createInfo.depthBiasSlopeFactor,
			/* lineWidth               */ 1.0f
		};
		
		VkPipelineRasterizationStateCreateInfo rasterizationStateWireframe = rasterizationState;
		rasterizationStateWireframe.polygonMode = VK_POLYGON_MODE_LINE;
		rasterizationStateWireframe.cullMode = VK_CULL_MODE_NONE;
		
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
		
		std::array<VkGraphicsPipelineCreateInfo, 2> pipelineCreateInfos;
		uint32_t numPipelines = 1;
		
		pipelineCreateInfos[0] =
		{
			/* sType               */ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			/* pNext               */ nullptr,
			/* flags               */ 0,
			/* stageCount          */ numStageCreateInfos,
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
			/* renderPass          */ renderPassInfo.m_renderPass,
			/* subpass             */ renderPassInfo.m_subpass,
			/* basePipelineHandle  */ VK_NULL_HANDLE,
			/* basePipelineIndex   */ -1
		};
		
		if (pipelineCreateInfos[0].pVertexInputState == nullptr)
		{
			static const VkPipelineVertexInputStateCreateInfo defaultVertexInputState = 
			{
				/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
				/* pNext                           */ nullptr,
				/* flags                           */ 0,
				/* vertexBindingDescriptionCount   */ 0,
				/* pVertexBindingDescriptions      */ nullptr,
				/* vertexAttributeDescriptionCount */ 0,
				/* pVertexAttributeDescriptions    */ nullptr
			};
			
			pipelineCreateInfos[0].pVertexInputState = &defaultVertexInputState;
		}
		
		if (createInfo.hasWireframeVariant)
		{
			pipelineCreateInfos[0].flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
			
			pipelineCreateInfos[numPipelines] = pipelineCreateInfos[0];
			pipelineCreateInfos[numPipelines].flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
			pipelineCreateInfos[numPipelines].basePipelineIndex = 0;
			pipelineCreateInfos[numPipelines].pRasterizationState = &rasterizationStateWireframe;
			
			numPipelines++;
		}
		
		VkPipeline pipelinesOut[] = { VK_NULL_HANDLE, VK_NULL_HANDLE };
		
		CheckResult(vkCreateGraphicsPipelines(vulkan.device, VK_NULL_HANDLE, numPipelines, pipelineCreateInfos.data(),
		                                      nullptr, pipelinesOut));
		
		m_pipeline = pipelinesOut[0];
		m_wireframePipeline = pipelinesOut[1];
	}
	
	void Shader::Bind(CommandBuffer& cb, Shader::BindModes mode) const
	{
		if (mode == BindModes::Wireframe && !m_wireframePipeline.IsNull())
		{
			cb.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_wireframePipeline);
		}
		else
		{
			cb.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipeline);
		}
	}
}
