#include "shader.h"
#include "shadermodules.h"
#include "../../vulkan/setlayoutsmanager.h"
#include <array>

namespace MCR
{
	Shader::Shader(RenderPassInfo renderPassInfo, const CreateInfo& createInfo)
	{
		bool hasSpecializations = !createInfo.specializations.empty();
		const size_t numPermutations = hasSpecializations ? static_cast<size_t>(createInfo.specializations.size()) : 1;
		m_pipelines.resize(static_cast<size_t>(numPermutations));
		if (createInfo.hasWireframeVariant)
		{
			m_wireframePipelines.resize(static_cast<size_t>(numPermutations));
		}
		
		//Allocates memory for descriptor set layouts.
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
		
		int stagesPerPermutation = 1;
		if (!createInfo.gsName.empty())
			stagesPerPermutation++;
		if (!createInfo.fsName.empty())
			stagesPerPermutation++;
		
		uint32_t numPipelineStageCI = static_cast<uint32_t>(stagesPerPermutation * numPermutations);
		void* stageCreateInfosMem = alloca(numPipelineStageCI * sizeof(VkPipelineShaderStageCreateInfo));
		auto stageCreateInfos = reinterpret_cast<VkPipelineShaderStageCreateInfo*>(stageCreateInfosMem);
		VkPipelineShaderStageCreateInfo* stageCreateInfosOut = stageCreateInfos;
		
		for (int p = 0; p < numPermutations; p++)
		{
			InitShaderStageCreateInfo(*stageCreateInfosOut, VK_SHADER_STAGE_VERTEX_BIT,
			                          GetShaderModule(createInfo.vsName));
			
			if (hasSpecializations)
			{
				stageCreateInfosOut->pSpecializationInfo = createInfo.specializations[p].vsSpecInfo;
			}
			stageCreateInfosOut++;
			
			if (!createInfo.gsName.empty())
			{
				InitShaderStageCreateInfo(*stageCreateInfosOut, VK_SHADER_STAGE_GEOMETRY_BIT,
				                          GetShaderModule(createInfo.gsName));
				
				if (hasSpecializations)
				{
					stageCreateInfosOut->pSpecializationInfo = createInfo.specializations[p].gsSpecInfo;
				}
				stageCreateInfosOut++;
			}
			
			if (!createInfo.fsName.empty())
			{
				InitShaderStageCreateInfo(*stageCreateInfosOut, VK_SHADER_STAGE_FRAGMENT_BIT,
				                          GetShaderModule(createInfo.fsName));
				
				if (hasSpecializations)
				{
					stageCreateInfosOut->pSpecializationInfo = createInfo.specializations[p].fsSpecInfo;
				}
				stageCreateInfosOut++;
			}
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
		
		VkPipelineDepthStencilStateCreateInfo depthStencilState = 
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
		
		if (createInfo.stencilState != nullptr)
		{
			depthStencilState.stencilTestEnable = VK_TRUE;
			depthStencilState.front = createInfo.stencilState->front;
			depthStencilState.back = createInfo.stencilState->back;
		}
		
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
		
		const VkPipelineVertexInputStateCreateInfo* vertexInputState = createInfo.vertexInputState;
		if (createInfo.vertexInputState == nullptr)
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
			
			vertexInputState = &defaultVertexInputState;
		}
		
		size_t numPipelineCreateInfos = numPermutations * (createInfo.hasWireframeVariant ? 2 : 1);
		void* pipelineCreateInfosMem = alloca(sizeof(VkGraphicsPipelineCreateInfo) * numPipelineCreateInfos);
		auto pipelineCreateInfos = reinterpret_cast<VkGraphicsPipelineCreateInfo*>(pipelineCreateInfosMem);
		
		for (size_t i = 0; i < numPermutations; i++)
		{
			pipelineCreateInfos[i] =
			{
				/* sType               */ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
				/* pNext               */ nullptr,
				/* flags               */ 0,
				/* stageCount          */ static_cast<uint32_t>(stagesPerPermutation),
				/* pStages             */ &stageCreateInfos[i * stagesPerPermutation],
				/* pVertexInputState   */ vertexInputState,
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
				/* basePipelineIndex   */ i == 0 ? -1 : 0
			};
			
			if (createInfo.hasWireframeVariant)
			{
				pipelineCreateInfos[numPermutations + i] = pipelineCreateInfos[i];
				pipelineCreateInfos[numPermutations + i].flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
				pipelineCreateInfos[numPermutations + i].basePipelineIndex = 0;
				pipelineCreateInfos[numPermutations + i].pRasterizationState = &rasterizationStateWireframe;
			}
		}
		
		if (numPipelineCreateInfos > 1)
		{
			pipelineCreateInfos[0].flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
		}
		
		
		VkPipeline* pipelinesOut = reinterpret_cast<VkPipeline*>(alloca(sizeof(VkPipeline) * numPipelineCreateInfos));
		
		CheckResult(vkCreateGraphicsPipelines(vulkan.device, VK_NULL_HANDLE,
		                                      static_cast<uint32_t>(numPipelineCreateInfos),
		                                      pipelineCreateInfos, nullptr, pipelinesOut));
		
		for (size_t i = 0; i < numPermutations; i++)
		{
			m_pipelines[i] = pipelinesOut[i];
			if (createInfo.hasWireframeVariant)
			{
				m_wireframePipelines[i] = pipelinesOut[numPermutations + i];
			}
		}
	}
	
	void Shader::Bind(CommandBuffer& cb, Shader::BindModes mode, uint32_t permutation) const
	{
		if (mode == BindModes::Wireframe && !m_wireframePipelines.empty())
		{
			cb.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_wireframePipelines[permutation]);
		}
		else
		{
			cb.BindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, *m_pipelines[permutation]);
		}
	}
}
