#pragma once

#include <glm/glm.hpp>

#include "../vulkan/vk.h"

namespace MCR
{
#pragma pack(push, 1)
	struct Vertex
	{
		glm::vec3 m_position;
		float _0;
		glm::vec3 m_normal;
		float _1;
		glm::vec3 m_texCoord;
		float _2;
	};
#pragma pack(pop)
	
	extern const VkPipelineVertexInputStateCreateInfo blockVertexInputState;
	extern const VkPipelineVertexInputStateCreateInfo blockVertexShadowInputState;
}
