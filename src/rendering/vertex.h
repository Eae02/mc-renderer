#pragma once

#include <glm/glm.hpp>

#include "../vulkan/vk.h"

namespace MCR
{
#pragma pack(push, 1)
	struct Vertex
	{
		glm::vec3 m_position;
		float m_roughness;
		glm::vec3 m_normal;
		float m_bendiness;
		glm::vec3 m_tangent;
		float _2;
		glm::vec4 m_texCoord;
	};
#pragma pack(pop)
	
	extern const VkPipelineVertexInputStateCreateInfo blockVertexInputState;
	extern const VkPipelineVertexInputStateCreateInfo blockVertexShadowInputState;
}
