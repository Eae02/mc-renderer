#include "skyshader.h"
#include "../blendstates.h"
#include "../framebuffer.h"

namespace MCR
{
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "Sky" };
	
	static const VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) };
	
	const Shader::CreateInfo SkyShader::s_createInfo = CreateInfo()
		.SetVertexShaderName("sky.vs")
		.SetFragmentShaderName("sky.fs")
		.SetPushConstantRanges(SingleElementSpan(pushConstantRange))
		.SetDSLayoutNames(setLayouts)
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::additive))
		.SetDynamicState(dynamicState);
	
	SkyShader::SkyShader(RenderPassInfo renderPassInfo, const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPassInfo, s_createInfo), m_descriptorSet("Sky")
	{
		VkWriteDescriptorSet renderSettingsWrite;
		
		m_descriptorSet.InitWriteDescriptorSet(renderSettingsWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       renderSettingsBufferInfo);
		
		UpdateDescriptorSets(SingleElementSpan(renderSettingsWrite));
	}
	
	void SkyShader::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet descriptorWrites[2];
		
		const VkDescriptorImageInfo depthImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetDepthImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[0], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       depthImageInfo);
		
		const VkDescriptorImageInfo godRaysImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetBlurredGodRaysImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[1], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       godRaysImageInfo);
		
		UpdateDescriptorSets(descriptorWrites);
	}
	
	void SkyShader::Bind(CommandBuffer& commandBuffer)
	{
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets);
		
		Shader::Bind(commandBuffer);
	}
}
