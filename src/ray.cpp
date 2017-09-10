#include "ray.h"

namespace MCR
{
	float Ray::GetDistanceToPoint(const glm::vec3& point) const
	{
		return glm::length(glm::cross(m_direction, point - m_start));
	}
	
	float Ray::ProjectPoint(const glm::vec3& point) const
	{
		return glm::dot(point - m_start, m_direction);
	}
}
