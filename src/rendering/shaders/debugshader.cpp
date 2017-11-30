#include "debugshader.h"
#include "../blendstates.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "DebugShader_Global" };
	
	static const VkVertexInputBindingDescription vertexInputBinding = 
	{
		/* binding   */ 0,
		/* stride    */ sizeof(float) * 3,
		/* inputRate */ VK_VERTEX_INPUT_RATE_VERTEX
	};
	
	static const VkVertexInputAttributeDescription vertexAttribute = { 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 };
	
	static const VkPipelineVertexInputStateCreateInfo vertexInputState = 
	{
		/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		/* pNext                           */ nullptr,
		/* flags                           */ 0,
		/* vertexBindingDescriptionCount   */ 1,
		/* pVertexBindingDescriptions      */ &vertexInputBinding,
		/* vertexAttributeDescriptionCount */ 1,
		/* pVertexAttributeDescriptions    */ &vertexAttribute
	};
	
	static const VkPushConstantRange pushConstantRange =
	{
		/* stageFlags */ VK_SHADER_STAGE_FRAGMENT_BIT,
		/* offset     */ 0,
		/* size       */ sizeof(float) * 4
	};
	
	const Shader::CreateInfo DebugShader::s_createInfo = CreateInfo()
		.SetVertexShaderName("debug.vs")
		.SetFragmentShaderName("debug.fs")
		.SetDSLayoutNames(setLayouts)
		.SetPushConstantRanges(SingleElementSpan(pushConstantRange))
		.SetVertexInputState(&vertexInputState)
		.SetEnableDepthClamp(true)
		.SetEnableDepthTest(true)
		.SetEnableDepthWrite(false)
		.SetHasWireframeVariant(true)
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::alphaBlend))
		.SetDynamicState(dynamicState);
	
	DebugShader::DebugShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPassInfo, s_createInfo), m_globalDescriptorSet("DebugShader_Global")
	{
		VkWriteDescriptorSet globalDescriptorWrite;
		
		m_globalDescriptorSet.InitWriteDescriptorSet(globalDescriptorWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             renderSettingsBufferInfo);
		
		UpdateDescriptorSets(SingleElementSpan(globalDescriptorWrite));
	}
	
	void DebugShader::Bind(CommandBuffer& cb, BindModes mode) const
	{
		Shader::Bind(cb, mode);
		
		const VkDescriptorSet descriptorSets[] = { *m_globalDescriptorSet };
		
		cb.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets, { });
	}
	
	void DebugShader::SetColor(CommandBuffer& cb, const glm::vec4& color) const
	{
		cb.PushConstants(GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(color), &color);
	}
}
