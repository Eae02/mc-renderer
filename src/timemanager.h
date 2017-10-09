#pragma once

#include <glm/glm.hpp>

#include "rendering/dirlight.h"

namespace MCR
{
	class TimeManager
	{
	public:
		TimeManager();
		
		void Update(float dt, const class InputState& inputState);
		
		void SetTimeScale(float timeScale);
		
		inline const glm::vec3& GetShadowDirection() const
		{
			return m_shadowDirection;
		}
		
		inline const DirLight& GetSunDescription() const
		{
			return m_sun;
		}
		
		inline const DirLight& GetMoonDescription() const
		{
			return m_moon;
		}
		
	private:
		float m_time = 0;
		float m_timeScale = 0;
		
		glm::vec3 m_shadowDirection;
		
		DirLight m_sun;
		DirLight m_moon;
	};
}
