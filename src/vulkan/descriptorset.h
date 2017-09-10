#pragma once

#include <vulkan/vulkan.h>
#include <string_view>

#include "vkhandle.h"

namespace MCR
{
	inline void UpdateDescriptorSets(gsl::span<const VkWriteDescriptorSet> descriptorWrites)
	{
		vkUpdateDescriptorSets(vulkan.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
	}
	
	inline void InitWriteDescriptorSet(VkWriteDescriptorSet& writeDescriptorSet, VkDescriptorSet set, uint32_t binding,
	                                   VkDescriptorType type, const VkDescriptorBufferInfo& bufferInfo,
	                                   uint32_t arrayElement = 0)
	{
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.pNext = nullptr;
		writeDescriptorSet.dstSet = set;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.dstArrayElement = arrayElement;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.pImageInfo = nullptr;
		writeDescriptorSet.pBufferInfo = &bufferInfo;
		writeDescriptorSet.pTexelBufferView = nullptr;
	}
	
	inline void InitWriteDescriptorSet(VkWriteDescriptorSet& writeDescriptorSet, VkDescriptorSet set, uint32_t binding,
	                                   VkDescriptorType type, const VkDescriptorImageInfo& imageInfo,
	                                   uint32_t arrayElement = 0)
	{
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.pNext = nullptr;
		writeDescriptorSet.dstSet = set;
		writeDescriptorSet.dstBinding = binding;
		writeDescriptorSet.dstArrayElement = arrayElement;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = type;
		writeDescriptorSet.pImageInfo = &imageInfo;
		writeDescriptorSet.pBufferInfo = nullptr;
		writeDescriptorSet.pTexelBufferView = nullptr;
	}
	
	class UniqueDescriptorSet
	{
	public:
		inline UniqueDescriptorSet()
		    : m_descriptorSet(VK_NULL_HANDLE) { }
		
		explicit UniqueDescriptorSet(std::string_view layoutName);
		
		inline void InitWriteDescriptorSet(VkWriteDescriptorSet& writeDescriptorSet, uint32_t binding,
		                                   VkDescriptorType type, const VkDescriptorBufferInfo& bufferInfo,
		                                   uint32_t arrayElement = 0)
		{
			MCR::InitWriteDescriptorSet(writeDescriptorSet, m_descriptorSet, binding, type, bufferInfo, arrayElement);
		}
		
		inline void InitWriteDescriptorSet(VkWriteDescriptorSet& writeDescriptorSet, uint32_t binding,
		                                   VkDescriptorType type, const VkDescriptorImageInfo& imageInfo,
		                                   uint32_t arrayElement = 0)
		{
			MCR::InitWriteDescriptorSet(writeDescriptorSet, m_descriptorSet, binding, type, imageInfo, arrayElement);
		}
		
		inline void Reset()
		{
			m_pool.Reset();
			m_descriptorSet = VK_NULL_HANDLE;
		}
		
		inline operator bool() const
		{
			return m_descriptorSet != VK_NULL_HANDLE;
		}
		
		inline VkDescriptorSet operator*() const
		{
			return m_descriptorSet;
		}
		
	private:
		VkHandle<VkDescriptorPool> m_pool;
		
		VkDescriptorSet m_descriptorSet;
	};
}
