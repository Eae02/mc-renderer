#include "skyshader.h"
#include "../blendstates.h"
#include "../framebuffer.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "Sky" };
	
	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	
	const Shader::CreateInfo SkyShader::s_createInfo = 
	{
		/* vsName                  */ "sky.vs",
		/* gsName                  */ "",
		/* fsName                  */ "sky.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ { },
		/* vertexInputState        */ &vertexInputState,
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
		/* attachmentBlendStates   */ SingleElementSpan(BlendStates::noBlending),
		/* dynamicState            */ dynamicState
	};
	
	SkyShader::SkyShader(VkRenderPass renderPass, const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPass, s_createInfo), m_descriptorSet("Sky")
	{
		VkWriteDescriptorSet renderSettingsWrite;
		
		m_descriptorSet.InitWriteDescriptorSet(renderSettingsWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       renderSettingsBufferInfo);
		
		UpdateDescriptorSets(SingleElementSpan(renderSettingsWrite));
	}
	
	void SkyShader::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet descriptorWrites[2];
		
		const VkDescriptorImageInfo colorImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetColorImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[0], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       colorImageInfo);
		
		const VkDescriptorImageInfo depthImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetDepthImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[1], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       depthImageInfo);
		
		UpdateDescriptorSets(descriptorWrites);
	}
	
	void SkyShader::Bind(CommandBuffer& commandBuffer)
	{
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets);
		
		Shader::Bind(commandBuffer);
	}
}
