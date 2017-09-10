#pragma once

#include <glm/glm.hpp>

namespace MCR
{
	class Ray
	{
	public:
		struct IntersectInfo
		{
			bool m_intersected;
			float m_distance;
		};
		
		Ray() = default;
		
		inline Ray(const glm::vec3& start, const glm::vec3& direction)
		    : m_start(start), m_direction(glm::normalize(direction)) { }
		
		inline static Ray FromStartEnd(const glm::vec3& start, const glm::vec3& end)
		{
			Ray ray;
			ray.m_start = start;
			ray.m_direction = end - start;
			return ray;
		}
		
		float GetDistanceToPoint(const glm::vec3& point) const;
		
		float ProjectPoint(const glm::vec3& point) const;
		
		inline const glm::vec3& GetStart() const
		{ return m_start; }
		
		inline void SetStart(const glm::vec3& start)
		{ m_start = start; }
		
		inline const glm::vec3& GetDirection() const
		{ return m_direction; }
		
		inline void SetDirection(const glm::vec3& direction)
		{ m_direction = direction; }
		
		inline glm::vec3 GetPoint(float distance) const
		{ return m_start + m_direction * distance; }
		
	private:
		glm::vec3 m_start;
		glm::vec3 m_direction;
	};
}
