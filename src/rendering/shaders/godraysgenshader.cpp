#include "godraysgenshader.h"
#include "../framebuffer.h"
#include "../blendstates.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "GodRaysGen" };
	
	const Shader::CreateInfo GodRaysGenShader::s_createInfo = 
	{
		/* vsName                  */ "godrays.vs",
		/* gsName                  */ "",
		/* fsName                  */ "godrays-gen.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ { },
		/* vertexInputState        */ nullptr,
		/* topology                */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport                */ { 0, 0, 1, 1, 0, 1 },
		/* scissor                 */ { 0, 0, 1, 1 },
		/* enableDepthClamp        */ false,
		/* cullMode                */ VK_CULL_MODE_NONE,
		/* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest         */ false,
		/* enableDepthWrite        */ false,
		/* stencilState            */ nullptr,
		/* hasWireframeVariant     */ false,
		/* depthCompareOp          */ VK_COMPARE_OP_LESS,
		/* enableDepthBias         */ false,
		/* depthBiasConstantFactor */ 0.0f,
		/* depthBiasClamp          */ 0.0f,
		/* depthBiasSlopeFactor    */ 0.0f,
		/* attachmentBlendStates   */ SingleElementSpan(BlendStates::noBlending),
		/* dynamicState            */ dynamicState
	};
	
	GodRaysGenShader::GodRaysGenShader(Shader::RenderPassInfo renderPass,
	                                   const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPass, s_createInfo), m_descriptorSet("GodRaysGen")
	{
		VkWriteDescriptorSet renderSettingsWrite;
		
		m_descriptorSet.InitWriteDescriptorSet(renderSettingsWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       renderSettingsBufferInfo);
		
		UpdateDescriptorSets(SingleElementSpan(renderSettingsWrite));
	}
	
	void GodRaysGenShader::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet descriptorWrite;
		
		const VkDescriptorImageInfo depthImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetDepthImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrite, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       depthImageInfo);
		
		UpdateDescriptorSets(SingleElementSpan(descriptorWrite));
	}
	
	void GodRaysGenShader::Bind(CommandBuffer& commandBuffer)
	{
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets);
		
		Shader::Bind(commandBuffer);
	}
}
