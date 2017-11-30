#include "uishader.h"
#include "uidrawlist.h"
#include "../rendering/blendstates.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "UI_Sampler", "UI_Image" };
	
	static const VkVertexInputBindingDescription vertexInputBinding = 
	{
		/* binding   */ 0,
		/* stride    */ sizeof(UIDrawList::Vertex),
		/* inputRate */ VK_VERTEX_INPUT_RATE_VERTEX
	};
	
	static const VkVertexInputAttributeDescription vertexAttributes[] = 
	{
		{ 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIDrawList::Vertex, m_position) },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(UIDrawList::Vertex, m_texCoord) },
		{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIDrawList::Vertex, m_color) }
	};
	
	static const VkPipelineVertexInputStateCreateInfo vertexInputState = 
	{
		/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		/* pNext                           */ nullptr,
		/* flags                           */ 0,
		/* vertexBindingDescriptionCount   */ 1,
		/* pVertexBindingDescriptions      */ &vertexInputBinding,
		/* vertexAttributeDescriptionCount */ ArrayLength(vertexAttributes),
		/* pVertexAttributeDescriptions    */ vertexAttributes
	};
	
	static const VkPushConstantRange pushConstantRange = 
	{
		/* stageFlags */ VK_SHADER_STAGE_VERTEX_BIT,
		/* offset     */ 0,
		/* size       */ sizeof(float) * 2
	};
	
	const Shader::CreateInfo UIShader::s_createInfo = Shader::CreateInfo()
		.SetVertexShaderName("ui.vs")
		.SetFragmentShaderName("ui.fs")
		.SetDSLayoutNames(setLayouts)
		.SetPushConstantRanges(SingleElementSpan(pushConstantRange))
		.SetVertexInputState(&vertexInputState)
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::alphaBlend))
		.SetDynamicState(dynamicState);
}
