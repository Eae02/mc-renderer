#pragma once

#include <glm/glm.hpp>

namespace MCR
{
	class AABoundingBox
	{
	public:
		inline AABoundingBox() { }
		
		inline AABoundingBox(glm::vec3 pos1, glm::vec3 pos2)
		    : m_positions{ pos1, pos2 } { }
		
		inline AABoundingBox(glm::vec3 pos, float w, float h, float d)
		    : m_positions{ pos, pos + glm::vec3(w, h, d) } { }
		
		bool Contains(glm::vec3 pos) const;
		
		inline glm::vec3 GetCenter() const
		{ return (MaxPos() + MinPos()) / 2.0f; }
		
		bool Intersects(AABoundingBox other) const;
		
		inline const glm::vec3& Pos1() const
		{ return m_positions[0]; }
		
		inline const glm::vec3& Pos2() const
		{ return m_positions[1]; }
		
		glm::vec3 GetNthVertex(int n) const;
		
		glm::vec3 MinPos() const;
		
		glm::vec3 MaxPos() const;
		
		inline void SetPos1(glm::vec3 pos)
		{ m_positions[0] = pos; }
		
		inline void SetPos2(glm::vec3 pos)
		{ m_positions[1] = pos; }
		
		inline float GetWidth() const
		{ return std::abs(m_positions[0].x - m_positions[1].x); }
		
		inline float GetHeight() const
		{ return std::abs(m_positions[0].y - m_positions[1].y); }
		
		inline float GetDepth() const
		{ return std::abs(m_positions[0].z - m_positions[1].z); }
		
		inline glm::vec3 Size() const
		{ return { GetWidth(), GetHeight(), GetDepth() }; }
		
	private:
		glm::vec3 m_positions[2];
	};
}
