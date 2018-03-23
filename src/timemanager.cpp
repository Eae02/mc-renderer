#include "timemanager.h"
#include "inputstate.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/color_space.hpp>

namespace MCR
{
	TimeManager::TimeManager()
	{
		SetTimeScale(60 * 60 * 0.1f);
		
		m_accumulatedTime = 0.2f;
	}
	
	void TimeManager::Update(float dt, const InputState& inputState)
	{
		if (m_freezeTime)
			return;
		
		m_accumulatedTime += dt * m_timeScale;
		
		//Sets m_time to the accumulated time modulo 2
		m_time = m_accumulatedTime - std::floor(m_accumulatedTime / 2.0f) * 2.0f;
		
		const float yaw = glm::radians(30.0f);
		
		float sunAngle;
		sunAngle = m_time * glm::pi<float>();
		
		glm::mat3 rotationMatrix(glm::rotate(glm::mat4(1), yaw, glm::vec3(0, 1, 0)) *
		                         glm::rotate(glm::mat4(1), sunAngle, glm::vec3(1, 0, 0)));
		
		float sunIntensity = std::max(-m_sun.m_direction.y, 0.0f);
		
		m_sun.m_direction = rotationMatrix[2];
		m_sun.m_radiance = glm::convertLinearToSRGB(glm::pow(glm::vec3(0.94f, 0.65f, 0.32f), glm::vec3(0.75f))) *
			glm::vec3(8.0f) * std::min(sunIntensity * 2.0f, 1.0f);
		
		m_moon.m_radiance = glm::vec3(0.9f, 0.9f, 1.0f) * 2.0f * (1.0f - sunIntensity);
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
	
	float TimeManager::GetStarIntensityScale() const
	{
		return 1.0f - glm::clamp(-m_sun.m_direction.y * 5.0f, 0.0f, 0.99f);
	}
}
