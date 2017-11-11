#include "timemanager.h"
#include "inputstate.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace MCR
{
	TimeManager::TimeManager()
	{
		SetTimeScale(60 * 60 * 3);
		
		m_time = 0.2f;
	}
	
	void TimeManager::Update(float dt, const InputState& inputState)
	{
		if (inputState.IsKeyPressed(Keys::UpArrow))
		{
			m_time = std::fmod(m_time + dt * 0.2f, 2.0f);
		}
		
		if (inputState.IsKeyPressed(Keys::DownArrow))
		{
			m_time = std::fmod(m_time - dt * 0.2f, 2.0f);
		}
		
		m_time = m_time - std::floor(m_time / 2.0f) * 2.0f;
		
		const float yaw = glm::radians(30.0f);
		
		float sunAngle;
		sunAngle = m_time * glm::pi<float>();
		
		glm::mat3 rotationMatrix(glm::rotate(glm::mat4(1), yaw, glm::vec3(0, 1, 0)) *
		                         glm::rotate(glm::mat4(1), sunAngle, glm::vec3(1, 0, 0)));
		
		float sunIntensity = std::max(-m_sun.m_direction.y, 0.0f);
		
		m_sun.m_direction = rotationMatrix[2];
		m_sun.m_radiance = glm::vec3(10) * std::min(sunIntensity * 2.5f, 1.0f);
		
		m_moon.m_radiance = glm::vec3(0.9f, 0.9f, 1.0f) * 0.5f * (1.0f - sunIntensity);
		m_moon.m_direction = -glm::normalize(glm::vec3(0.2f, 1.5f, 0.8f));
		
		if (sunIntensity > 0.1f)
		{
			m_shadowDirection = m_sun.m_direction;
		}
		else
		{
			m_shadowDirection = m_moon.m_direction;
		}
	}
	
	void TimeManager::SetTimeScale(float timeScale)
	{
		m_timeScale = timeScale / (60 * 60 * 12);
	}
}
