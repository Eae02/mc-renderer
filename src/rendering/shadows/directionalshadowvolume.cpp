#include "directionalshadowvolume.h"
#include "../frustum.h"

#include <algorithm>

namespace MCR
{
	DirectionalShadowVolume::DirectionalShadowVolume(const glm::mat4* invLightMatrices, const glm::vec3& lightDirection)
	    : m_lightDirection(lightDirection)
	{
		std::transform(invLightMatrices, invLightMatrices + DirLightCascades, m_cascadeFrusta.begin(),
		               [&] (const glm::mat4& invLightMatrix)
		{
			return Frustum(invLightMatrix);
		});
	}
	
	bool DirectionalShadowVolume::Intersects(const AABoundingBox& aabb) const
	{
		return std::any_of(m_cascadeFrusta.begin(), m_cascadeFrusta.end(), [&] (const Frustum& frustum)
		{
			return frustum.Intersects(aabb);
		});
	}
}
