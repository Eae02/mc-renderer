#include "timemanager.h"
#include "inputstate.h"

#include <glm/gtc/constants.hpp>

namespace MCR
{
	TimeManager::TimeManager()
	{
		SetTimeScale(60 * 60 * 3);
		
		m_time = 0.2f;
	}
	
	void TimeManager::Update(float dt, const InputState& inputState)
	{
		//m_time = std::fmod(m_time + dt * m_timeScale, 2.0f);
		
		if (inputState.IsKeyPressed(Keys::UpArrow))
		{
			m_time = std::fmod(m_time + dt * 0.2f, 2.0f);
		}
		
		if (inputState.IsKeyPressed(Keys::DownArrow))
		{
			m_time = std::fmod(m_time - dt * 0.2f, 2.0f);
		}
		
		
		float sunAngle;
		sunAngle = m_time * glm::pi<float>();
		m_sun.m_direction = { -glm::cos(sunAngle), -glm::sin(sunAngle), 0.0f };
		m_sun.m_radiance = glm::vec3(5.0f) * std::min(-m_sun.m_direction.y * 2.5f, 1.0f);
		
		m_moon.m_radiance = glm::vec3(0.7f, 0.7f, 1.0f) * 0.5f * GetMoonIntensity();
		m_moon.m_direction = glm::normalize(glm::vec3(1, 1, 1));
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
