#pragma once

#include <glm/glm.hpp>

namespace MCR
{
	struct ViewProjection
	{
		glm::mat4 m_proj;
		glm::mat4 m_invProj;
		
		glm::mat4 m_view;
		glm::mat4 m_invView;
		
		glm::mat4 m_viewProj;
		glm::mat4 m_invViewProj;
	};
}
