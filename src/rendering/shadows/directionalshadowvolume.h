#pragma once

#include <glm/glm.hpp>
#include <array>

#include "../constants.h"
#include "../frustum.h"

namespace MCR
{
	class DirectionalShadowVolume
	{
	public:
		DirectionalShadowVolume() = default;
		
		DirectionalShadowVolume(const glm::mat4* invLightMatrices, const glm::vec3& lightDirection);
		
		bool Intersects(const AABoundingBox& aabb) const;
		
		inline const glm::vec3& GetLightDirection() const
		{ return m_lightDirection; }
		
		inline const Frustum& GetFrustum(size_t cascade) const
		{ return m_cascadeFrusta[cascade]; }
		
	private:
		glm::vec3 m_lightDirection;
		
		std::array<Frustum, DirLightCascades> m_cascadeFrusta;
	};
}
