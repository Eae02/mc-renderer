#include "plane.h"

namespace MCR
{
	Plane::Plane(const glm::vec3& normal, float distance)
	{
		const float nLength = normal.length();
		m_distance = distance / nLength;
		m_normal = normal / nLength;
	}
	
	Plane::Plane(const glm::vec3& normal, const glm::vec3& point)
	{
		m_normal = glm::normalize(normal);
		m_distance = glm::dot(point, m_normal);
	}
	
	Plane::Plane(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
	{
		const glm::vec3 v1 = b - a;
		const glm::vec3 v2 = c - a;
		
		m_normal = glm::normalize(glm::cross(v1, v2));
		
		m_distance = glm::dot(a, m_normal);
	}
	
	glm::vec3 Plane::GetClosestPointOnPlane(const glm::vec3& point) const
	{
		//A plane that has the same normal as this plane and contains 'point'.
		Plane parallelPlane(m_normal, point);
		
		float distDiff = parallelPlane.m_distance - m_distance;
		return point - m_normal * distDiff;
	}
	
	glm::mat3 Plane::GetTBNMatrix(glm::vec3 forward) const
	{
		glm::vec3 tangent = glm::cross(m_normal, glm::normalize(forward));
		return glm::mat3(tangent, glm::cross(tangent, m_normal), m_normal);
	}
	
	Ray::IntersectInfo Plane::GetIntersection(const Ray& ray) const
	{
		const float div = glm::dot(ray.GetDirection(), m_normal);
		if (std::abs(div) < 1E-9)
			return { false, std::numeric_limits<float>::quiet_NaN() };
		
		const glm::vec3 w = (m_normal * m_distance) - ray.GetStart();
		const float dist = glm::dot(w, m_normal) / div;
		
		return { dist > 0, dist };
	}
	
	Ray Plane::GetIntersection(const Plane& other, bool& parallel) const
	{
		glm::vec3 dir = glm::cross(other.m_normal, m_normal);
		glm::vec3 absDir = glm::abs(dir);
		
		//Checks if the planes are parallel.
		if ((parallel = (absDir.x + absDir.y + absDir.z < std::numeric_limits<float>::epsilon())))
			return { };
		
		//Determines the dimension of dir with the greatest absolute value.
		int maxDim;
		if (absDir.x > absDir.y)
		{
			if (absDir.x > absDir.z)
				maxDim = 0;
			else
				maxDim = 2;
		}
		else
		{
			if (absDir.y > absDir.z)
				maxDim = 1;
			else
				maxDim = 2;
		}
		
		glm::vec3 point;
		
		switch (maxDim)
		{
		case 0:                     // intersect with x=0
			point.y = (m_distance * other.m_normal.z - other.m_distance * m_normal.z) / dir.x;
			point.z = (other.m_distance * m_normal.y - m_distance * other.m_normal.y) / dir.x;
			break;
		case 1:                     // intersect with y=0
			point.x = (other.m_distance * m_normal.z - m_distance * other.m_normal.z) / dir.y;
			point.z = (m_distance * other.m_normal.x - other.m_distance * m_normal.x) / dir.y;
			break;
		case 2:                     // intersect with z=0
			point.x = (m_distance * other.m_normal.y - other.m_distance * m_normal.y) / dir.z;
			point.y = (other.m_distance * m_normal.x - m_distance * other.m_normal.x) / dir.z;
			break;
		}
		
		return { point, dir };
	}
	
	void Plane::FlipNormal()
	{
		m_normal *= -1;
		m_distance *= -1;
	}
}
