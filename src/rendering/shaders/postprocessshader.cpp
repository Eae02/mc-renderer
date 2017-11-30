#include "postprocessshader.h"
#include "../framebuffer.h"
#include "../blendstates.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "PostProcess" };
	
	const Shader::CreateInfo PostProcessShader::s_createInfo = CreateInfo()
		.SetVertexShaderName("post.vs")
		.SetFragmentShaderName("post.fs")
		.SetDSLayoutNames(setLayouts)
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::noBlending))
		.SetDynamicState(dynamicState);
	
	PostProcessShader::PostProcessShader(RenderPassInfo renderPassInfo)
	    : Shader(renderPassInfo, s_createInfo), m_descriptorSet("PostProcess")
	{
		
	}
	
	void PostProcessShader::Bind(CommandBuffer& commandBuffer)
	{
		Shader::Bind(commandBuffer);
		
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets);
	}
	
	void PostProcessShader::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet descriptorWrite;
		
		const VkDescriptorImageInfo inputImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetWaterColorImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrite, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       inputImageInfo);
		
		UpdateDescriptorSets(SingleElementSpan(descriptorWrite));
	}
}
