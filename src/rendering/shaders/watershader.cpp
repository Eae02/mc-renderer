#include "watershader.h"
#include "../regions/watermesh.h"
#include "../blendstates.h"
#include "../framebuffer.h"
#include "../../loadcontext.h"
#include "../causticstexture.h"

#include <stb_image.h>
#include <glm/gtc/constants.hpp>

namespace MCR
{
	constexpr size_t NORMAL_MAP_SAMPLES = 3;
	constexpr size_t NM_ROTATIONS_BUFFER_SIZE = sizeof(float) * 8 * NORMAL_MAP_SAMPLES;
	
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "Water", "ShadowSample" };
	
	static const VkPushConstantRange pushConstantRanges[] = { { VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) } };
	
	const Shader::CreateInfo WaterShader::s_createInfo =
	{
		/* vsName                  */ "water.vs",
		/* gsName                  */ "",
		/* fsName                  */ "water.fs",
		/* setLayoutNames          */ setLayouts,
		/* pushConstantRanges      */ pushConstantRanges,
		/* vertexInputState        */ &WaterMesh::s_vertexInputState,
		/* topology                */ VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		/* viewport                */ { 0, 0, 1, 1, 0, 1 },
		/* scissor                 */ { 0, 0, 1, 1 },
		/* enableDepthClamp        */ false,
		/* cullMode                */ VK_CULL_MODE_NONE,
		/* frontFace               */ VK_FRONT_FACE_CLOCKWISE,
		/* enableDepthTest         */ true,
		/* enableDepthWrite        */ true,
		/* hasWireframeVariant     */ true,
		/* depthCompareOp          */ VK_COMPARE_OP_LESS,
		/* enableDepthBias         */ false,
		/* depthBiasConstantFactor */ 0.0f,
		/* depthBiasClamp          */ 0.0f,
		/* depthBiasSlopeFactor    */ 0.0f,
		/* attachmentBlendStates   */ SingleElementSpan(BlendStates::noBlending),
		/* dynamicState            */ dynamicState
	};
	
	WaterShader::WaterShader(Shader::RenderPassInfo renderPassInfo,
	                         const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPassInfo, s_createInfo), m_descriptorSet("Water")
	{
		// ** Creates the normal map transforms buffer **
		const VmaAllocationCreateInfo deviceMemoryAllocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		VkBufferCreateInfo deviceBufferCreateInfo;
		InitBufferCreateInfo(deviceBufferCreateInfo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT, NM_ROTATIONS_BUFFER_SIZE);
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &deviceBufferCreateInfo, &deviceMemoryAllocationCI,
		                            m_normalMapTransformsBuffer.GetCreateAddress(),
		                            m_normalMapTransformsAllocation.GetCreateAddress(), nullptr));
		
		// ** Writes buffers to the descriptor set **
		VkWriteDescriptorSet bufferDSWrites[2];
		
		m_descriptorSet.InitWriteDescriptorSet(bufferDSWrites[0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       renderSettingsBufferInfo);
		
		VkDescriptorBufferInfo normalMapTransformInfo = { *m_normalMapTransformsBuffer, 0, NM_ROTATIONS_BUFFER_SIZE };
		m_descriptorSet.InitWriteDescriptorSet(bufferDSWrites[1], 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       normalMapTransformInfo);
		
		UpdateDescriptorSets(bufferDSWrites);
	}
	
	void WaterShader::Bind(CommandBuffer& cb, VkDescriptorSet shadowDescriptorSet, bool underwater) const
	{
		Shader::Bind(cb);
		
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet, shadowDescriptorSet };
		
		cb.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets, { });
		
		const uint32_t pushConstantData = underwater ? VK_TRUE : VK_FALSE;
		cb.PushConstants(GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &pushConstantData);
	}
	
	void WaterShader::FramebufferChanged(const Framebuffer& framebuffer)
	{
		VkWriteDescriptorSet descriptorWrites[2];
		
		const VkDescriptorImageInfo colorImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetWaterInputColorImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[0], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       colorImageInfo);
		
		const VkDescriptorImageInfo depthImageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ framebuffer.GetWaterInputDepthImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(descriptorWrites[1], 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       depthImageInfo);
		
		UpdateDescriptorSets(descriptorWrites);
	}
	
	void WaterShader::Initialize(LoadContext& loadContext)
	{
		// ** Loads the water normal map **
		fs::path waterNormalsPath = GetResourcePath() / "textures" / "water_normals.png";
		std::string waterNormalsPathStr = waterNormalsPath.string();
		
		int nmWidth, nmHeight;
		stbi_uc* waterNormalsData = stbi_load(waterNormalsPathStr.c_str(), &nmWidth, &nmHeight, nullptr, 4);
		const size_t waterNormalsBytes = static_cast<const size_t>(nmWidth * nmHeight * 4);
		
		// ** Creates the normal map image **
		const VkFormat normalMapFormat = VK_FORMAT_R8G8B8A8_UNORM;
		VkImageCreateInfo normalMapCreateInfo;
		InitImageCreateInfo(normalMapCreateInfo, VK_IMAGE_TYPE_2D, normalMapFormat, static_cast<uint32_t>(nmWidth),
		                    static_cast<uint32_t>(nmHeight));
		normalMapCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		
		VmaAllocationCreateInfo imageAllocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		vmaCreateImage(vulkan.allocator, &normalMapCreateInfo, &imageAllocationCI, m_normalMap.GetCreateAddress(),
		               m_normalMapAllocation.GetCreateAddress(), nullptr);
		
		// ** Creates the normal map image view **
		VkImageViewCreateInfo normalMapViewCreateInfo;
		InitImageViewCreateInfo(normalMapViewCreateInfo, *m_normalMap, VK_IMAGE_VIEW_TYPE_2D, normalMapFormat,
		                        { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		vkCreateImageView(vulkan.device, &normalMapViewCreateInfo, nullptr, m_normalMapView.GetCreateAddress());
		
		// ** Writes the normal map to the descriptor set **
		VkWriteDescriptorSet normalMapDSWrite;
		
		VkDescriptorImageInfo normalMapInfo = 
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ *m_normalMapView,
			/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(normalMapDSWrite, 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		                                       normalMapInfo);
		UpdateDescriptorSets(SingleElementSpan(normalMapDSWrite));
		
		// ** Creates the staging buffer **
		const VkDeviceSize stagingBufferSize = NM_ROTATIONS_BUFFER_SIZE + waterNormalsBytes;
		VmaAllocation stagingAllocation;
		VkBuffer stagingBuffer;
		void* stagingBufferMemory;
		CreateStagingBuffer(stagingBufferSize, &stagingBuffer, &stagingAllocation, &stagingBufferMemory);
		
		// ** Computes normal map rotations **
		const float rotations[NORMAL_MAP_SAMPLES] = { glm::radians(-35.0f), glm::radians(0.0f), glm::radians(35.0f) };
		float* normalMapRotationsMemory = reinterpret_cast<float*>(stagingBufferMemory);
		for (size_t i = 0; i < NORMAL_MAP_SAMPLES; i++)
		{
			float cosRot = std::cos(rotations[i]);
			float sinRot = std::sin(rotations[i]);
			
			normalMapRotationsMemory[i * 8 + 0] = cosRot;
			normalMapRotationsMemory[i * 8 + 1] = sinRot;
			normalMapRotationsMemory[i * 8 + 4] = -sinRot;
			normalMapRotationsMemory[i * 8 + 5] = cosRot;
		}
		
		// ** Copies normal map image data to the staging buffer **
		memcpy(reinterpret_cast<char*>(stagingBufferMemory) + NM_ROTATIONS_BUFFER_SIZE,
		       waterNormalsData, waterNormalsBytes);
		
		// ** Transitions the normal map to TRANSFER_DST_OPTIMAL **
		VkImageMemoryBarrier preNormalMapCopyBarrier;
		InitImageMemoryBarrier(preNormalMapCopyBarrier, *m_normalMap);
		preNormalMapCopyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		preNormalMapCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		preNormalMapCopyBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
		                                    { }, { }, SingleElementSpan(preNormalMapCopyBarrier));
		
		// ** Uploads normal map image data **
		const VkBufferImageCopy normalMapCopyRegion =
		{
			/* bufferOffset      */ NM_ROTATIONS_BUFFER_SIZE,
			/* bufferRowLength   */ 0,
			/* bufferImageHeight */ 0,
			/* imageSubresource  */ { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
			/* imageOffset       */ { 0, 0, 0 },
			/* imageExtent       */ { static_cast<uint32_t>(nmWidth), static_cast<uint32_t>(nmHeight), 1 }
		};
		loadContext.GetCB().CopyBufferToImage(stagingBuffer, *m_normalMap, normalMapCopyRegion);
		
		// ** Transitions the normal map to SHADER_READ_ONLY_OPTIMAL **
		VkImageMemoryBarrier postNormalMapCopyBarrier;
		InitImageMemoryBarrier(postNormalMapCopyBarrier, *m_normalMap);
		postNormalMapCopyBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		postNormalMapCopyBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		postNormalMapCopyBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		postNormalMapCopyBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
		                                    { }, { }, SingleElementSpan(postNormalMapCopyBarrier));
		
		// ** Uploads normal map transforms **
		const VkBufferCopy copyRegion = { 0, 0, NM_ROTATIONS_BUFFER_SIZE };
		loadContext.GetCB().CopyBuffer(stagingBuffer, *m_normalMapTransformsBuffer, copyRegion);
		
		const VkBufferMemoryBarrier normalMapTransformsBarrier =
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_UNIFORM_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_normalMapTransformsBuffer,
			/* offset              */ 0,
			/* size                */ VK_WHOLE_SIZE
		};
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
		                                    { }, SingleElementSpan(normalMapTransformsBarrier), { });
		
		loadContext.TakeResource(VkHandle<VmaAllocation>(stagingAllocation));
		loadContext.TakeResource(VkHandle<VkBuffer>(stagingBuffer));
	}
	
	void WaterShader::SetCausticsTexture(const CausticsTexture& causticsTexture)
	{
		VkWriteDescriptorSet dsWrite;
		
		const VkDescriptorImageInfo imageInfo =
		{
			/* sampler     */ VK_NULL_HANDLE, //Immutable
			/* imageView   */ causticsTexture.GetImageView(),
			/* imageLayout */ VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
		m_descriptorSet.InitWriteDescriptorSet(dsWrite, 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageInfo);
		
		UpdateDescriptorSets(SingleElementSpan(dsWrite));
	}
}
