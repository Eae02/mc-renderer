#pragma once

#include <glm/glm.hpp>

namespace MCR
{
#pragma pack(push, 1)
	struct DirLight
	{
		glm::vec3 m_direction;
		float _0;
		glm::vec3 m_radiance;
		float _1;
	};
#pragma pack(pop)
}
