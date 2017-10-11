#include "uishader.h"
#include "uidrawlist.h"
#include "../rendering/blendstates.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "UI_Sampler", "UI_Image" };
	
	VkVertexInputBindingDescription vertexInputBinding = 
	{
		/* binding   */ 0,
		/* stride    */ sizeof(UIDrawList::Vertex),
		/* inputRate */ VK_VERTEX_INPUT_RATE_VERTEX
	};
	
	VkVertexInputAttributeDescription vertexAttributes[] = 
	{
		{ 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(UIDrawList::Vertex, m_position) },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(UIDrawList::Vertex, m_texCoord) },
		{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(UIDrawList::Vertex, m_color) }
	};
	
	const VkPipelineVertexInputStateCreateInfo blockVertexInputState = 
	{
		/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		/* pNext                           */ nullptr,
		/* flags                           */ 0,
		/* vertexBindingDescriptionCount   */ 1,
		/* pVertexBindingDescriptions      */ &vertexInputBinding,
		/* vertexAttributeDescriptionCount */ ArrayLength(vertexAttributes),
		/* pVertexAttributeDescriptions    */ vertexAttributes
	};
	
	const VkPushConstantRange pushConstantRange = 
	{
		/* stageFlags */ VK_SHADER_STAGE_VERTEX_BIT,
		/* offset     */ 0,
		/* size       */ sizeof(float) * 2
	};
	
	const Shader::CreateInfo UIShader::s_createInfo = 
	{
		/* vsName                  */ "ui.vs",
		/* gsName                  */ "",
		/* fsName                  */ "ui.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ SingleElementSpan(pushConstantRange),
		/* vertexInputState        */ &blockVertexInputState,
		/* topology                */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport                */ { 0, 0, 1, 1, 0, 1 },
		/* scissor                 */ { 0, 0, 1, 1 },
		/* enableDepthClamp        */ false,
		/* cullMode                */ VK_CULL_MODE_NONE,
		/* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest         */ false,
		/* enableDepthWrite        */ false,
		/* hasWireframeVariant     */ false,
		/* depthCompareOp          */ VK_COMPARE_OP_LESS,
		/* enableDepthBias         */ false,
		/* depthBiasConstantFactor */ 0.0f,
		/* depthBiasClamp          */ 0.0f,
		/* depthBiasSlopeFactor    */ 0.0f,
		/* attachmentBlendStates   */ SingleElementSpan(BlendStates::alphaBlend),
		/* dynamicState            */ dynamicState
	};
}
