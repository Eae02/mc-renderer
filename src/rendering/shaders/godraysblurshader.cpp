#include "godraysblurshader.h"
#include "../framebuffer.h"
#include "../blendstates.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "GodRaysBlur" };
	
	static const VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) };
	
	const Shader::CreateInfo GodRaysBlurShader::s_createInfo = CreateInfo()
		.SetVertexShaderName("godrays.vs")
		.SetFragmentShaderName("godrays-hblur.fs")
		.SetDSLayoutNames(setLayouts)
		.SetPushConstantRanges(SingleElementSpan(pushConstantRange))
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::noBlending))
		.SetDynamicState(dynamicState);
	
	GodRaysBlurShader::GodRaysBlurShader(Shader::RenderPassInfo renderPass)
	    : Shader(renderPass, s_createInfo), m_descriptorSet("GodRaysBlur")
	{
		
	}
	
	void GodRaysBlurShader::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet descriptorWrite;
		
		const VkDescriptorImageInfo inputImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetUnblurredGodRaysImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrite, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       inputImageInfo);
		
		UpdateDescriptorSets(SingleElementSpan(descriptorWrite));
	}
	
	void GodRaysBlurShader::Bind(CommandBuffer& commandBuffer)
	{
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets);
		
		Shader::Bind(commandBuffer);
	}
}
