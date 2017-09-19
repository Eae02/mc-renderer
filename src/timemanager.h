#pragma once

#include <glm/glm.hpp>

namespace MCR
{
	class TimeManager
	{
	public:
		TimeManager();
		
		void Update(float dt);
		
		void SetTimeScale(float timeScale);
		
		inline const glm::vec3& GetDLDirection() const
		{ return m_dlDirection; }
		inline const glm::vec3& GetDLRadiance() const
		{ return m_dlRadiance; }
		
		float GetMoonIntensity() const;
		
	private:
		float m_time = 0;
		float m_timeScale = 0;
		
		glm::vec3 m_dlDirection;
		glm::vec3 m_dlRadiance;
	};
}
