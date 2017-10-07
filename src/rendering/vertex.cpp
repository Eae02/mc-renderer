#include "vertex.h"

namespace MCR
{
	VkVertexInputBindingDescription blockVertexInputBinding = 
	{
		/* binding   */ 0,
		/* stride    */ sizeof(Vertex),
		/* inputRate */ VK_VERTEX_INPUT_RATE_VERTEX
	};
	
	const VkVertexInputAttributeDescription blockVertexAttributes[] = 
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_position) },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_normal) },
		{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_texCoord) }
	};
	
	const VkPipelineVertexInputStateCreateInfo blockVertexInputState = 
	{
		/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		/* pNext                           */ nullptr,
		/* flags                           */ 0,
		/* vertexBindingDescriptionCount   */ 1,
		/* pVertexBindingDescriptions      */ &blockVertexInputBinding,
		/* vertexAttributeDescriptionCount */ ArrayLength(blockVertexAttributes),
		/* pVertexAttributeDescriptions    */ blockVertexAttributes
	};
	
	const VkVertexInputAttributeDescription blockVertexShadowAttributes[] = 
	{
		{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_position) },
		{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, m_texCoord) }
	};
	
	const VkPipelineVertexInputStateCreateInfo blockVertexShadowInputState = 
	{
		/* sType                           */ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		/* pNext                           */ nullptr,
		/* flags                           */ 0,
		/* vertexBindingDescriptionCount   */ 1,
		/* pVertexBindingDescriptions      */ &blockVertexInputBinding,
		/* vertexAttributeDescriptionCount */ ArrayLength(blockVertexShadowAttributes),
		/* pVertexAttributeDescriptions    */ blockVertexShadowAttributes
	};
}
