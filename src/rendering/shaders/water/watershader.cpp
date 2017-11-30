#include "watershader.h"
#include "../../regions/watermesh.h"
#include "../../blendstates.h"
#include "../../framebuffer.h"
#include "../../../loadcontext.h"
#include "../../causticstexture.h"

#include <stb_image.h>
#include <random>
#include <glm/gtc/constants.hpp>

namespace MCR
{
	constexpr size_t NORMAL_MAP_SAMPLES = 3;
	constexpr size_t NM_ROTATIONS_SIZE = sizeof(float) * 8 * NORMAL_MAP_SAMPLES;
	constexpr size_t WAVES_SIZE = WaterShader::WaveCount * sizeof(WaterShader::Wave);
	constexpr size_t DATA_BUFFER_SIZE = NM_ROTATIONS_SIZE + WAVES_SIZE;
	
	static const VkDynamicState dynamicState[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	
	static const std::string_view setLayouts[] = { "Water", "ShadowSample" };
	
	static const VkStencilOpState stencilOpState = 
	{
		/* failOp      */ VK_STENCIL_OP_REPLACE,
		/* passOp      */ VK_STENCIL_OP_REPLACE,
		/* depthFailOp */ VK_STENCIL_OP_KEEP,
		/* compareOp   */ VK_COMPARE_OP_ALWAYS,
		/* compareMask */ 0x0,
		/* writeMask   */ 0x1,
		/* reference   */ 0x1
	};
	
	static const Shader::StencilState stencilState = { stencilOpState, stencilOpState };
	
	static const VkSpecializationMapEntry specializationMapEntries[] =
	{
		{ 0, 0, sizeof(uint32_t) },
		{ 1, sizeof(uint32_t), sizeof(uint32_t) }
	};
	
	static const uint32_t aboveWaterSpecData[] = { 0, WaterShader::WaveCount };
	
	static const VkSpecializationInfo aboveWaterSpecInfo =
	{
		static_cast<uint32_t>(ArrayLength(specializationMapEntries)),
		specializationMapEntries,
		sizeof(aboveWaterSpecData),
		aboveWaterSpecData
	};
	
	static const uint32_t underwaterSpecData[] = { 1, WaterShader::WaveCount };
	
	static const VkSpecializationInfo belowWaterSpecInfo =
	{
		static_cast<uint32_t>(ArrayLength(specializationMapEntries)),
		specializationMapEntries,
		sizeof(underwaterSpecData),
		underwaterSpecData
	};
	
	static const Shader::Specialization specializations[] =
	{
		{
			/* vs  */ &aboveWaterSpecInfo,
			/* tcs */ nullptr,
			/* tes */ nullptr,
			/* gs  */ nullptr,
			/* fs  */ &aboveWaterSpecInfo
		},
		{
			/* vs  */ &belowWaterSpecInfo,
			/* tcs */ nullptr,
			/* tes */ nullptr,
			/* gs  */ nullptr,
			/* fs  */ &belowWaterSpecInfo
		}
	};
	
	const Shader::CreateInfo WaterShader::s_createInfo = CreateInfo()
		.SetVertexShaderName("water-tess.vs")
		.SetTessControlShaderName("water.tcs")
		.SetTessEvaluationShaderName("water.tes")
		.SetFragmentShaderName("water.fs")
		.SetDSLayoutNames(setLayouts)
		.SetVertexInputState(&WaterMesh::s_vertexInputState)
		.SetTopology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)
		.SetPatchControlPoints(3)
		.SetEnableDepthClamp(true)
		.SetEnableDepthTest(true)
		.SetEnableDepthWrite(true)
		.SetStencilState(&stencilState)
		.SetHasWireframeVariant(true)
		.SetAttachmentBlendStates(SingleElementSpan(BlendStates::noBlending))
		.SetDynamicState(dynamicState)
		.SetSpecializations(specializations);
	
	WaterShader::WaterShader(Shader::RenderPassInfo renderPassInfo,
	                         const VkDescriptorBufferInfo& renderSettingsBufferInfo)
	    : Shader(renderPassInfo, s_createInfo), m_descriptorSet("Water")
	{
		// ** Creates the data uniform buffer **
		const VmaAllocationCreateInfo deviceMemoryAllocationCI = { 0, VMA_MEMORY_USAGE_GPU_ONLY };
		
		VkBufferCreateInfo dataBufferCreateInfo;
		InitBufferCreateInfo(dataBufferCreateInfo, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
		                     VK_BUFFER_USAGE_TRANSFER_DST_BIT, DATA_BUFFER_SIZE);
		
		CheckResult(vmaCreateBuffer(vulkan.allocator, &dataBufferCreateInfo, &deviceMemoryAllocationCI,
		                            m_dataUniformBuffer.GetCreateAddress(),
		                            m_normalMapTransformsAllocation.GetCreateAddress(), nullptr));
		
		// ** Writes buffers to the descriptor set **
		VkWriteDescriptorSet bufferDSWrites[2];
		
		m_descriptorSet.InitWriteDescriptorSet(bufferDSWrites[0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       renderSettingsBufferInfo);
		
		VkDescriptorBufferInfo dataBufferInfo = { *m_dataUniformBuffer, 0, DATA_BUFFER_SIZE };
		m_descriptorSet.InitWriteDescriptorSet(bufferDSWrites[1], 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		                                       dataBufferInfo);
		
		UpdateDescriptorSets(bufferDSWrites);
	}
	
	void WaterShader::Bind(CommandBuffer& cb, VkDescriptorSet shadowDescriptorSet,
	                       bool underwater, BindModes bindMode) const
	{
		Shader::Bind(cb, bindMode, underwater ? 1 : 0);
		
		const VkDescriptorSet descriptorSets[] = { *m_descriptorSet, shadowDescriptorSet };
		
		cb.BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, GetLayout(), 0, descriptorSets, { });
	}
	
	void WaterShader::FramebufferChanged(const Framebuffer& framebuffer)
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
		const VkDeviceSize stagingBufferSize = DATA_BUFFER_SIZE + waterNormalsBytes;
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
		
		// ** Computes wave parameters **
		std::mt19937 wavesRand;
		std::uniform_real_distribution<float> wavelengthDist(1.0f, 16.0f);
		
		Wave* waves = reinterpret_cast<Wave*>(reinterpret_cast<char*>(stagingBufferMemory) + NM_ROTATIONS_SIZE);
		float totalAmplitude = 0;
		for (size_t i = 0; i < WaveCount; i++)
		{
			float iProgress = static_cast<float>(i) / static_cast<float>(WaveCount - 1);
			const float theta = iProgress * glm::two_pi<float>();
			
			float wavelength = wavelengthDist(wavesRand);
			
			waves[i].m_direction = glm::vec2(std::cos(theta), std::sin(theta));
			waves[i].m_amplitude = wavelength;
			waves[i].m_freq = glm::two_pi<float>() / wavelength;
			waves[i].m_speed = 1.5f * waves[i].m_freq;
			
			totalAmplitude += waves[i].m_amplitude;
		}
		
		float amplitudeScale = 0.4f / totalAmplitude;
		for (size_t i = 0; i < WaveCount; i++)
		{
			waves[i].m_amplitude *= amplitudeScale;
		}
		
		// ** Copies normal map image data to the staging buffer **
		memcpy(reinterpret_cast<char*>(stagingBufferMemory) + DATA_BUFFER_SIZE,
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
			/* bufferOffset      */ DATA_BUFFER_SIZE,
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
		
		// ** Uploads data buffer contents **
		const VkBufferCopy copyRegion = { 0, 0, DATA_BUFFER_SIZE };
		loadContext.GetCB().CopyBuffer(stagingBuffer, *m_dataUniformBuffer, copyRegion);
		
		const VkBufferMemoryBarrier dataUniformBufferBarrier =
		{
			/* sType               */ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
			/* pNext               */ nullptr,
			/* srcAccessMask       */ VK_ACCESS_TRANSFER_WRITE_BIT,
			/* dstAccessMask       */ VK_ACCESS_UNIFORM_READ_BIT,
			/* srcQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* dstQueueFamilyIndex */ VK_QUEUE_FAMILY_IGNORED,
			/* buffer              */ *m_dataUniformBuffer,
			/* offset              */ 0,
			/* size                */ VK_WHOLE_SIZE
		};
		
		loadContext.GetCB().PipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0,
		                                    { }, SingleElementSpan(dataUniformBufferBarrier), { });
		
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
	
	float WaterShader::GetWaterHeightOffset(glm::vec2 position, float t) const
	{
		float height = 0;
		
		for (const Wave& wave : m_waves)
		{
			const float theta = t * wave.m_speed + glm::dot(position, wave.m_direction) * wave.m_freq;
			height += std::sin(theta) * wave.m_amplitude;
		}
		
		return height;
	}
}
