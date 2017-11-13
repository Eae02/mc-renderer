#include "causticstexture.h"
#include "shaders/shadermodules.h"
#include "../vulkan/instance.h"

namespace MCR
{
	static constexpr uint32_t resolution = 512;
	static constexpr uint32_t depth = 128;
	static constexpr VkFormat format = VK_FORMAT_R8_UNORM;
	static constexpr float scale = 16;
	
	VkHandle<VkPipelineLayout> CausticsTexture::s_genPipelineLayout;
	VkHandle<VkPipeline> CausticsTexture::s_genPipeline;
	uint32_t CausticsTexture::s_workGroupSize;
	
	CausticsTexture::CausticsTexture()
	    : m_generateDescriptorSet("CausticsGen")
	{
		// ** Creates the image **
		VkImageCreateInfo imageCreateInfo;
		InitImageCreateInfo(imageCreateInfo, VK_IMAGE_TYPE_3D, format, resolution, resolution, depth);
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		
		VmaAllocationCreateInfo allocationCreateInfo = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		CheckResult(vmaCreateImage(vulkan.allocator, &imageCreateInfo, &allocationCreateInfo,
		                           m_image.GetCreateAddress(), m_allocation.GetCreateAddress(), nullptr));
		
		// ** Creates the image view **
		VkImageViewCreateInfo viewCreateInfo;
		InitImageViewCreateInfo(viewCreateInfo, *m_image, VK_IMAGE_VIEW_TYPE_3D, format,
		                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		CheckResult(vkCreateImageView(vulkan.device, &viewCreateInfo, nullptr, m_imageView.GetCreateAddress()));
		
		// ** Writes the image to the descriptor set **
		VkWriteDescriptorSet dsWrite;
		const VkDescriptorImageInfo imageInfo = { VK_NULL_HANDLE, *m_imageView, VK_IMAGE_LAYOUT_GENERAL };
		m_generateDescriptorSet.InitWriteDescriptorSet(dsWrite, 0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imageInfo);
		UpdateDescriptorSets(SingleElementSpan(dsWrite));
	}
	
	struct SpecializationData
	{
		uint32_t m_workGroupSize[3];
		float m_positionScaleXY;
		float m_positionScaleZ;
	};
	
	void CausticsTexture::CreatePipelines()
	{
		s_workGroupSize = static_cast<uint32_t>(std::pow(vulkan.limits.maxComputeWorkGroupInvocations, 1.0 / 3.0));
		for (uint32_t maxWorkGroupSizeDim : vulkan.limits.maxComputeWorkGroupSize)
		{
			if (s_workGroupSize > maxWorkGroupSizeDim)
			{
				s_workGroupSize = maxWorkGroupSizeDim;
			}
		}
		
		//Rounds the work group size down to the previous power of two.
		s_workGroupSize = 1u << static_cast<uint32_t>(std::log2(s_workGroupSize));
		
		Log("Caustics work group size: ", s_workGroupSize);
		
		// ** Creates the pipeline layout **
		
		VkDescriptorSetLayout setLayout = GetDescriptorSetLayout("CausticsGen");
		
		const VkPipelineLayoutCreateInfo layoutCreateInfo = 
		{
			/* sType                  */ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			/* pNext                  */ nullptr,
			/* flags                  */ 0,
			/* setLayoutCount         */ 1,
			/* pSetLayouts            */ &setLayout,
			/* pushConstantRangeCount */ 0,
			/* pPushConstantRanges    */ nullptr
		};
		
		VkPipelineLayout pipelineLayout;
		CheckResult(vkCreatePipelineLayout(vulkan.device, &layoutCreateInfo, nullptr, &pipelineLayout));
		s_genPipelineLayout = pipelineLayout;
		
		// ** Creates the pipeline **
		
		SpecializationData specializationData;
		std::fill(MAKE_RANGE(specializationData.m_workGroupSize), s_workGroupSize);
		specializationData.m_positionScaleXY = scale / static_cast<float>(resolution);
		specializationData.m_positionScaleZ = scale / static_cast<float>(depth);
		
		const VkSpecializationMapEntry specializationMapEntries[] = 
		{
			{ 0, offsetof(SpecializationData, m_workGroupSize), sizeof(SpecializationData::m_workGroupSize) },
			{ 1, offsetof(SpecializationData, m_positionScaleXY), sizeof(SpecializationData::m_positionScaleXY) },
			{ 2, offsetof(SpecializationData, m_positionScaleZ), sizeof(SpecializationData::m_positionScaleZ) }
		};
		
		const VkSpecializationInfo specializationInfo = 
		{
			/* mapEntryCount */ static_cast<uint32_t>(ArrayLength(specializationMapEntries)),
			/* pMapEntries   */ specializationMapEntries,
			/* dataSize      */ sizeof(SpecializationData),
			/* pData         */ &specializationData
		};
		
		VkComputePipelineCreateInfo pipelineCreateInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		pipelineCreateInfo.stage = 
		{
			/* sType               */ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			/* pNext               */ nullptr,
			/* flags               */ 0,
			/* stage               */ VK_SHADER_STAGE_COMPUTE_BIT,
			/* module              */ GetShaderModule("caustics.cs"),
			/* pName               */ "main",
			/* pSpecializationInfo */ &specializationInfo
		};
		pipelineCreateInfo.layout = pipelineLayout;
		pipelineCreateInfo.basePipelineIndex = -1;
		
		CheckResult(vkCreateComputePipelines(vulkan.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr,
		                                     s_genPipeline.GetCreateAddress()));
	}
	
	void CausticsTexture::Render(CommandBuffer& commandBuffer)
	{
		// ** Changes the image layout to GENERAL **
		VkImageMemoryBarrier barrier;
		InitImageMemoryBarrier(barrier, *m_image);
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
		                              { }, { }, SingleElementSpan(barrier));
		
		// ** Generates caustics **
		
		commandBuffer.BindPipeline(VK_PIPELINE_BIND_POINT_COMPUTE, *s_genPipeline);
		
		const VkDescriptorSet descriptorSets[] = { *m_generateDescriptorSet };
		commandBuffer.BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, *s_genPipelineLayout, 0, descriptorSets);
		
		const uint32_t dispatchXY = DivRoundUp(resolution, s_workGroupSize);
		commandBuffer.Dispatch(dispatchXY, dispatchXY, DivRoundUp(depth, s_workGroupSize));
		
		// ** Changes the image layout to SHADER_READ_ONLY_OPTIMAL **
		barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		
		commandBuffer.PipelineBarrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                              { }, { }, SingleElementSpan(barrier));
	}
}
