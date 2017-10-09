#pragma once

#include <glm/glm.hpp>

#include "../plane.h"
#include "../aaboundingbox.h"

namespace MCR
{
	class AABoundingBox;
	class BoundingSphere;
	
	class Frustum
	{
	public:
		enum class Planes
		{
			NEAR = 0, FAR = 1, LEFT = 2, RIGHT = 3, UP = 4, DOWN = 5
		};
		
		Frustum() = default;
		explicit Frustum(const glm::mat4& inverseViewProj);
		
		bool Intersects(const AABoundingBox& aabb) const;
		
		bool Intersects(const AABoundingBox& aabb, const glm::mat4& transform) const;
		
		bool Contains(const glm::vec3& point) const;
		
		inline void SetEnableZCheck(bool value)
		{ m_enableZCheck = value; }
		inline bool EnableZCheck() const
		{ return m_enableZCheck; }
		
		inline const Plane& GetPlane(Planes p) const
		{ return m_planes[static_cast<int>(p)]; }
		
		inline const glm::vec3& GetCorner(size_t n) const
		{ return m_corners[n]; }
		
	private:
		glm::vec3 m_corners[8];
		
		Plane m_planes[6];
		bool m_enableZCheck = true;
	};
}
