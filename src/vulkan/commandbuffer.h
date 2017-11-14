#pragma once

#include <vulkan/vulkan.h>

#include "vkutils.h"

#include <gsl/span>

namespace MCR
{
	class CommandBuffer
	{
	public:
		inline CommandBuffer()
		    : m_commandBuffer(VK_NULL_HANDLE) { }
		
		explicit CommandBuffer(VkCommandPool pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		
		virtual ~CommandBuffer();
		
		inline CommandBuffer(CommandBuffer&& other)
		    : m_commandPool(other.m_commandPool), m_commandBuffer(other.m_commandBuffer)
		{
			other.m_commandBuffer = VK_NULL_HANDLE;
		}
		
		inline CommandBuffer& operator=(CommandBuffer other)
		{
			std::swap(m_commandBuffer, other.m_commandBuffer);
			std::swap(m_commandPool, other.m_commandPool);
			return *this;
		}
		
		inline const VkCommandBuffer& GetVkCB() const
		{
			return m_commandBuffer;
		}
		
		inline void Begin(VkCommandBufferUsageFlags usageFlags,
		                  const VkCommandBufferInheritanceInfo* inheritanceInfo = nullptr)
		{
			VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
			beginInfo.flags = usageFlags;
			beginInfo.pInheritanceInfo = inheritanceInfo;
			CheckResult(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));
		}
		
		inline void End()
		{
			CheckResult(vkEndCommandBuffer(m_commandBuffer));
		}
		
		inline void BindPipeline(VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
		{
			vkCmdBindPipeline(m_commandBuffer, pipelineBindPoint, pipeline);
		}
		
		inline void SetViewport(uint32_t firstViewport, gsl::span<const VkViewport> viewports)
		{
			vkCmdSetViewport(m_commandBuffer, firstViewport, viewports.size(), viewports.data());
		}
		
		inline void SetScissor(uint32_t firstScissor, gsl::span<const VkRect2D> scissors)
		{
			vkCmdSetScissor(m_commandBuffer, firstScissor, scissors.size(), scissors.data());
		}
		
		inline void SetStencilCompareMask(VkStencilFaceFlags faceMask, uint32_t compareMask)
		{
			vkCmdSetStencilCompareMask(m_commandBuffer, faceMask, compareMask);
		}
		
		inline void SetStencilWriteMask(VkStencilFaceFlags faceMask, uint32_t writeMask)
		{
			vkCmdSetStencilWriteMask(m_commandBuffer, faceMask, writeMask);
		}
		
		inline void SetStencilReference(VkStencilFaceFlags faceMask, uint32_t reference)
		{
			vkCmdSetStencilReference(m_commandBuffer, faceMask, reference);
		}
		
		inline void BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet,
		                               gsl::span<const VkDescriptorSet> descriptorSets)
		{
			vkCmdBindDescriptorSets(m_commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSets.size(),
			                        descriptorSets.data(), 0, nullptr);
		}
		
		inline void BindDescriptorSets(VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet,
		                               gsl::span<const VkDescriptorSet> descriptorSets, gsl::span<const uint32_t> dynamicOffsets)
		{
			vkCmdBindDescriptorSets(m_commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSets.size(),
			                        descriptorSets.data(), dynamicOffsets.size(), dynamicOffsets.data());
		}
		
		inline void BindIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
		{
			vkCmdBindIndexBuffer(m_commandBuffer, buffer, offset, indexType);
		}
		
		inline void BindVertexBuffers(uint32_t firstBinding, uint32_t bindingCount, const VkBuffer* buffers,
		                              const VkDeviceSize* offsets)
		{
			vkCmdBindVertexBuffers(m_commandBuffer, firstBinding, bindingCount, buffers, offsets);
		}
		
		inline void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
		{
			vkCmdDraw(m_commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
		}
		
		inline void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
		                        uint32_t firstInstance)
		{
			vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
		}
		
		inline void DrawIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
		{
			vkCmdDrawIndirect(m_commandBuffer, buffer, offset, drawCount, stride);
		}
		
		inline void DrawIndexedIndirect(VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
		{
			vkCmdDrawIndexedIndirect(m_commandBuffer, buffer, offset, drawCount, stride);
		}
		
		inline void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
		{
			vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
		}
		
		inline void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, const VkBufferCopy& region)
		{
			vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, 1, &region);
		}
		
		inline void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, gsl::span<const VkBufferCopy> regions)
		{
			vkCmdCopyBuffer(m_commandBuffer, srcBuffer, dstBuffer, regions.size(), regions.data());
		}
		
		inline void CopyImage(VkImage srcImage, VkImage dstImage, const VkImageCopy& region)
		{
			vkCmdCopyImage(m_commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			               dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}
		
		inline void CopyImage(VkImage srcImage, VkImage dstImage, gsl::span<const VkImageCopy> regions)
		{
			vkCmdCopyImage(m_commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			               dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
		}
		
		inline void BlitImage(VkImage srcImage, VkImage dstImage, const VkImageBlit& region, VkFilter filter)
		{
			vkCmdBlitImage(m_commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			               dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, filter);
		}
		
		inline void BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
		                      VkImageLayout dstImageLayout, const VkImageBlit& region, VkFilter filter)
		{
			vkCmdBlitImage(m_commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, 1, &region, filter);
		}
		
		inline void BlitImage(VkImage srcImage, VkImage dstImage, gsl::span<const VkImageBlit> regions, VkFilter filter)
		{
			vkCmdBlitImage(m_commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			               dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data(), filter);
		}
		
		inline void BlitImage(VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
		                      VkImageLayout dstImageLayout, gsl::span<const VkImageBlit> regions, VkFilter filter)
		{
			vkCmdBlitImage(m_commandBuffer, srcImage, srcImageLayout, dstImage,
			               dstImageLayout, regions.size(), regions.data(), filter);
		}
		
		inline void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, const VkBufferImageCopy& region)
		{
			vkCmdCopyBufferToImage(m_commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                       1, &region);
		}
		
		inline void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, gsl::span<const VkBufferImageCopy> regions)
		{
			vkCmdCopyBufferToImage(m_commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                       regions.size(), regions.data());
		}
		
		inline void CopyImageToBuffer(VkImage srcImage, VkBuffer dstBuffer, const VkBufferImageCopy& region)
		{
			vkCmdCopyImageToBuffer(m_commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer,
			                       1, &region);
		}
		
		inline void CopyImageToBuffer(VkImage srcImage, VkBuffer dstBuffer, gsl::span<const VkBufferImageCopy> regions)
		{
			vkCmdCopyImageToBuffer(m_commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer,
			                       regions.size(), regions.data());
		}
		
		inline void UpdateBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize, const void* data)
		{
			vkCmdUpdateBuffer(m_commandBuffer, dstBuffer, dstOffset, dataSize, data);
		}
		
		inline void FillBuffer(VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize size, uint32_t data)
		{
			vkCmdFillBuffer(m_commandBuffer, dstBuffer, dstOffset, size, data);
		}
		
		inline void ClearColorImage(VkImage image, const VkClearColorValue* color,
		                            const VkImageSubresourceRange& range)
		{
			vkCmdClearColorImage(m_commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, color, 1, &range);
		}
		
		inline void ClearColorImage(VkImage image, const VkClearColorValue* color,
		                            gsl::span<const VkImageSubresourceRange> ranges)
		{
			vkCmdClearColorImage(m_commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, color,
			                     ranges.size(), ranges.data());
		}
		
		inline void ClearDepthStencilImage(VkImage image, const VkClearDepthStencilValue* depthStencil,
		                                   gsl::span<const VkImageSubresourceRange> ranges)
		{
			vkCmdClearDepthStencilImage(m_commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                            depthStencil, ranges.size(), ranges.data());
		}
		
		inline void ClearDepthStencilImage(VkImage image, const VkClearDepthStencilValue* depthStencil,
		                                   const VkImageSubresourceRange& range)
		{
			vkCmdClearDepthStencilImage(m_commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			                            depthStencil, 1, &range);
		}
		
		inline void ClearAttachments(gsl::span<const VkClearAttachment> attachments, gsl::span<const VkClearRect> rects)
		{
			vkCmdClearAttachments(m_commandBuffer, attachments.size(), attachments.data(), rects.size(), rects.data());
		}
		
		inline void PipelineBarrier(VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
		                            VkDependencyFlags dependencyFlags, gsl::span<const VkMemoryBarrier> memoryBarriers,
		                            gsl::span<const VkBufferMemoryBarrier> bufferMemoryBarriers,
		                            gsl::span<const VkImageMemoryBarrier> imageMemoryBarriers)
		{
			vkCmdPipelineBarrier(m_commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarriers.size(),
			                     memoryBarriers.data(), bufferMemoryBarriers.size(), bufferMemoryBarriers.data(),
			                     imageMemoryBarriers.size(), imageMemoryBarriers.data());
		}
		
		inline void BeginQuery(VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
		{
			vkCmdBeginQuery(m_commandBuffer, queryPool, query, flags);
		}
		
		inline void EndQuery(VkQueryPool queryPool, uint32_t query)
		{
			vkCmdEndQuery(m_commandBuffer, queryPool, query);
		}
		
		inline void ResetQueryPool(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
		{
			vkCmdResetQueryPool(m_commandBuffer, queryPool, firstQuery, queryCount);
		}
		
		inline void WriteTimestamp(VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t query)
		{
			vkCmdWriteTimestamp(m_commandBuffer, pipelineStage, queryPool, query);
		}
		
		inline void CopyQueryPoolResults(VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
		                                 VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize stride,
		                                 VkQueryResultFlags flags)
		{
			vkCmdCopyQueryPoolResults(m_commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset,
			                          stride, flags);
		}
		
		inline void PushConstants(VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t offset,
		                          uint32_t size, const void* values)
		{
			vkCmdPushConstants(m_commandBuffer, layout, stageFlags, offset, size, values);
		}
		
		inline void BeginRenderPass(const VkRenderPassBeginInfo* renderPassBegin,
		                            VkSubpassContents contents = VK_SUBPASS_CONTENTS_INLINE)
		{
			vkCmdBeginRenderPass(m_commandBuffer, renderPassBegin, contents);
		}
		
		inline void NextSubpass(VkSubpassContents contents)
		{
			vkCmdNextSubpass(m_commandBuffer, contents);
		}
		
		inline void EndRenderPass()
		{
			vkCmdEndRenderPass(m_commandBuffer);
		}
		
		inline void ExecuteCommands(VkCommandBuffer commandBuffer)
		{
			vkCmdExecuteCommands(m_commandBuffer, 1, &commandBuffer);
		}
		
		inline void ExecuteCommands(gsl::span<const VkCommandBuffer> commandBuffers)
		{
			vkCmdExecuteCommands(m_commandBuffer, commandBuffers.size(), commandBuffers.data());
		}
		
	private:
		VkCommandPool m_commandPool;
		VkCommandBuffer m_commandBuffer;
	};
}
