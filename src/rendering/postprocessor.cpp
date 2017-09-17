#include "postprocessor.h"
#include "shaders/shadermodules.h"
#include "rendererframebuffer.h"

namespace MCR
{
	const VkSamplerCreateInfo depthSamplerCreateInfo = 
	{
		/* sType                   */ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		/* pNext                   */ nullptr,
		/* flags                   */ 0,
		/* magFilter               */ VK_FILTER_NEAREST,
		/* minFilter               */ VK_FILTER_NEAREST,
		/* mipmapMode              */ VK_SAMPLER_MIPMAP_MODE_LINEAR,
		/* addressModeU            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeV            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* addressModeW            */ VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		/* mipLodBias              */ 0.0f,
		/* anisotropyEnable        */ VK_FALSE,
		/* maxAnisotropy           */ 0,
		/* compareEnable           */ VK_FALSE,
		/* compareOp               */ VK_COMPARE_OP_ALWAYS,
		/* minLod                  */ 0.0f,
		/* maxLod                  */ 0,
		/* borderColor             */ VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		/* unnormalizedCoordinates */ VK_TRUE
	};
	
	PostProcessor::PostProcessor()
	    : m_commandBuffer(vulkan.stdCommandPools[QUEUE_FAMILY_GRAPHICS]),
	      m_descriptorSet("PostProcess")
	{
		const VkDescriptorSetLayout dsLayouts[] = 
		{
			GetDescriptorSetLayout("PostProcess")
		};
		
		const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = 
		{
			/* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			/* pNext                  */ nullptr,
			/* flags                  */ 0,
			/* setLayoutCount         */ gsl::narrow<uint32_t>(ArrayLength(dsLayouts)),
			/* pSetLayouts            */ dsLayouts,
			/* pushConstantRangeCount */ 0,
			/* pPushConstantRanges    */ nullptr
		};
		
		VkPipelineLayout pipelineLayout;
		CheckResult(vkCreatePipelineLayout(vulkan.device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
		m_pipelineLayout = pipelineLayout;
		
		VkComputePipelineCreateInfo pipelineCreateInfo =
		{
			/* sType              */ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			/* pNext              */ nullptr,
			/* flags              */ 0,
			/* stage              */ { },
			/* layout             */ pipelineLayout,
			/* basePipelineHandle */ nullptr,
			/* basePipelineIndex  */ -1
		};
		
		InitShaderStageCreateInfo(pipelineCreateInfo.stage, VK_SHADER_STAGE_COMPUTE_BIT, GetShaderModule("post.cs"));
		
		// ** Creates the depth sampler **
		CheckResult(vkCreateSampler(vulkan.device, &depthSamplerCreateInfo, nullptr,
		                            m_depthSampler.GetCreateAddress()));
		
		VkPipeline pipeline;
		CheckResult(vkCreateComputePipelines(vulkan.device, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));
		m_pipeline = pipeline;
	}
	
	void PostProcessor::SetRenderSettings(const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	{
		VkWriteDescriptorSet dsWrite;
		m_descriptorSet.InitWriteDescriptorSet(dsWrite, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, renderSettingsBufferInfo);
		UpdateDescriptorSets(SingleElementSpan(dsWrite));
	}
	
	void PostProcessor::FramebufferChanged(const RendererFramebuffer& framebuffer)
	{
		VkWriteDescriptorSet dsWrites[2];
		
		VkDescriptorImageInfo colorImageInfo = { nullptr, framebuffer.GetColorImageView(), VK_IMAGE_LAYOUT_GENERAL };
		m_descriptorSet.InitWriteDescriptorSet(dsWrites[0], 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, colorImageInfo);
		
		VkDescriptorImageInfo depthImageInfo = { *m_depthSampler, framebuffer.GetDepthImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		m_descriptorSet.InitWriteDescriptorSet(dsWrites[1], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, depthImageInfo);
		
		UpdateDescriptorSets(dsWrites);
		
		m_commandBuffer.Begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
		
		m_commandBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *m_pipeline);
		
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet };
		m_commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *m_pipelineLayout, 0, descriptorSets);
		
		m_commandBuffer.Dispatch(DivRoundUp(framebuffer.GetWidth(), 32u), DivRoundUp(framebuffer.GetHeight(), 32u), 1);
		
		VkImageMemoryBarrier barrier;
		InitImageMemoryBarrier(barrier, framebuffer.GetColorImage());
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		
		m_commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
		                                VK_DEPENDENCY_BY_REGION_BIT, { }, { }, SingleElementSpan(barrier));
		
		m_commandBuffer.End();
	}
}
