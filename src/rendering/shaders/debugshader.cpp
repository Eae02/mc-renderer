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
	
	static const VkPipelineVertexInputStateCreateInfo blockVertexInputState = 
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
	
	const Shader::CreateInfo DebugShader::s_createInfo = 
	{
		/* vsName                  */ "debug.vs",
		/* gsName                  */ "",
		/* fsName                  */ "debug.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ SingleElementSpan(pushConstantRange),
		/* vertexInputState        */ &blockVertexInputState,
		/* topology                */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport                */ { 0, 0, 1, 1, 0, 1 },
		/* scissor                 */ { 0, 0, 1, 1 },
		/* enableDepthClamp        */ true,
		/* cullMode                */ VK_CULL_MODE_NONE,
		/* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest         */ true,
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
	
	DebugShader::DebugShader(VkRenderPass renderPass, const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPass, s_createInfo), m_globalDescriptorSet("DebugShader_Global")
	{
		VkWriteDescriptorSet globalDescriptorWrite;
		
		m_globalDescriptorSet.InitWriteDescriptorSet(globalDescriptorWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                             renderSettingsBufferInfo);
		
		UpdateDescriptorSets(SingleElementSpan(globalDescriptorWrite));
	}
	
	void DebugShader::Bind(CommandBuffer& cb) const
	{
		Shader::Bind(cb, BindModes::Default);
		
		const VkDescriptorSet descriptorSets[] = { *m_globalDescriptorSet };
		
		cb.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets, { });
	}
	
	void DebugShader::SetColor(CommandBuffer& cb, const glm::vec4& color) const
	{
		cb.PushConstants(GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(color), &color);
	}
}
