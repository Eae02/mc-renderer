#include "timemanager.h"

#include <glm/gtc/constants.hpp>

namespace MCR
{
	TimeManager::TimeManager()
	{
		SetTimeScale(60 * 60 * 3);
	}
	
	void TimeManager::Update(float dt)
	{
		m_time = std::fmod(m_time + dt * m_timeScale, 2.0f);
		
		float sunAngle;
		sunAngle = m_time * glm::pi<float>();
		m_dlRadiance = glm::vec3(1.0f);
		
		m_dlDirection = { -glm::cos(sunAngle), -glm::sin(sunAngle), 0.0f };
	}
	
	void TimeManager::SetTimeScale(float timeScale)
	{
		m_timeScale = timeScale / (60 * 60 * 12);
	}
	
	float TimeManager::GetMoonIntensity() const
	{
		if (m_time < 0)
			return 0;
		return 1;
	}
}
