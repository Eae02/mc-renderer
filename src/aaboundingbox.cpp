#include "aaboundingbox.h"

#include <algorithm>

namespace MCR
{
	bool AABoundingBox::Contains(glm::vec3 pos) const
	{
		return pos.x > std::fmin(m_positions[0].x, m_positions[1].x) &&
		       pos.x < std::fmax(m_positions[0].x, m_positions[1].x) &&
		       pos.y > std::fmin(m_positions[0].y, m_positions[1].y) &&
		       pos.y < std::fmax(m_positions[0].y, m_positions[1].y) &&
		       pos.z > std::fmin(m_positions[0].z, m_positions[1].z) &&
		       pos.z < std::fmax(m_positions[0].z, m_positions[1].z);
	}
	
	bool AABoundingBox::Intersects(AABoundingBox other) const
	{
		return !(
			std::fmin(other.m_positions[0].x, other.m_positions[1].x) > std::fmax(m_positions[0].x, m_positions[1].x) ||
			std::fmax(other.m_positions[0].x, other.m_positions[1].x) < std::fmin(m_positions[0].x, m_positions[1].x) ||
			
			std::fmin(other.m_positions[0].y, other.m_positions[1].y) > std::fmax(m_positions[0].y, m_positions[1].y) ||
			std::fmax(other.m_positions[0].y, other.m_positions[1].y) < std::fmin(m_positions[0].y, m_positions[1].y) ||
			
			std::fmin(other.m_positions[0].z, other.m_positions[1].z) > std::fmax(m_positions[0].z, m_positions[1].z) ||
			std::fmax(other.m_positions[0].z, other.m_positions[1].z) < std::fmin(m_positions[0].z, m_positions[1].z)
		);
	}
	
	glm::vec3 AABoundingBox::GetNthVertex(int n) const
	{
		const bool useX2 = (n % 2) > 0;
		const bool useY2 = (n % 4) >= 2;
		const bool useZ2 = n >= 4;
		
		return { m_positions[useX2 ? 1 : 0].x, m_positions[useY2 ? 1 : 0].y, m_positions[useZ2 ? 1 : 0].z };
	}
	
	glm::vec3 AABoundingBox::MinPos() const
	{
		return {
			std::min(m_positions[0].x, m_positions[1].x),
			std::min(m_positions[0].y, m_positions[1].y),
			std::min(m_positions[0].z, m_positions[1].z)
		};
	}
	
	glm::vec3 AABoundingBox::MaxPos() const
	{
		return {
			std::max(m_positions[0].x, m_positions[1].x),
			std::max(m_positions[0].y, m_positions[1].y),
			std::max(m_positions[0].z, m_positions[1].z)
		};
	}
}
