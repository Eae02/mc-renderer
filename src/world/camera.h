#pragma once

#include <glm/glm.hpp>

namespace MCR
{
	class Camera
	{
	public:
		Camera();
		
		void Update(float dt, const class InputState& inputState);
		
		inline const glm::vec3& GetPosition() const
		{
			return m_position;
		}
		
		inline glm::vec3 GetForward() const
		{
			return m_rotationMatrix[2];
		}
		
		inline glm::vec3 GetLeft() const
		{
			return m_rotationMatrix[0];
		}
		
		inline glm::vec3 GetUp() const
		{
			return m_rotationMatrix[1];
		}
		
		inline const glm::mat4& GetViewMatrix() const
		{
			return m_viewMatrix;
		}
		
		inline const glm::mat4& GetInverseViewMatrix() const
		{
			return m_invViewMatrix;
		}
		
	private:
		glm::vec3 m_position;
		glm::vec2 m_rotation;
		
		glm::vec3 m_velocity;
		
		glm::mat4 m_invRotationMatrix;
		glm::mat4 m_rotationMatrix;
		
		glm::mat4 m_viewMatrix;
		glm::mat4 m_invViewMatrix;
	};
}
