#include "camera.h"
#include "../inputstate.h"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace MCR
{
	Camera::Camera()
	    : m_position(0, 160, 0), m_rotation(0, 0), m_velocity(0)
	{
		
	}
	
	void Camera::Update(float dt, const InputState& inputState)
	{
		const glm::vec2 mouseSensitivity(0.005f, 0.005f);
		
		m_rotation += inputState.GetCursorDelta() * mouseSensitivity;
		m_rotation.y = glm::clamp(m_rotation.y, -glm::half_pi<float>(), glm::half_pi<float>());
		
		m_invRotationMatrix = glm::rotate(glm::mat4(), m_rotation.y, glm::vec3(1, 0, 0)) *
		                      glm::rotate(glm::mat4(), m_rotation.x, glm::vec3(0, 1, 0));
		m_rotationMatrix = glm::transpose(m_invRotationMatrix);
		
		const float accelerationAmount = 20.0f;
		const float dragAmount = 3.0f;
		
		const glm::vec3 back = glm::vec3(m_rotationMatrix[2]);
		const glm::vec3 right = glm::vec3(m_rotationMatrix[0]);
		
		glm::vec3 force(0.0f);
		
		if (inputState.IsKeyPressed(Keys::W))
		{
			force -= back;
		}
		if (inputState.IsKeyPressed(Keys::S))
		{
			force += back;
		}
		if (inputState.IsKeyPressed(Keys::A))
		{
			force -= right;
		}
		if (inputState.IsKeyPressed(Keys::D))
		{
			force += right;
		}
		
		float forceMag = glm::length(force);
		if (forceMag > 1E-6f)
		{
			force *= accelerationAmount / forceMag;
		}
		
		force -= m_velocity * dragAmount;
		
		const float mass = 0.5f;
		
		const glm::vec3 oldVelocity = m_velocity;
		
		m_velocity += (force / mass) * dt;
		
		const float MaxSpeed = 5;
		float speed = glm::length(m_velocity);
		if (speed > MaxSpeed)
		{
			m_velocity *= MaxSpeed / speed;
		}
		
		float moveSpeed = inputState.IsKeyPressed(Keys::Q) ? 3.0f : 1.0f;
		
		m_position += (m_velocity + oldVelocity) * 0.5f * dt * moveSpeed;
		
		m_viewMatrix = glm::scale(glm::mat4(), glm::vec3(1, -1, 1)) *
		               m_invRotationMatrix *
		               glm::translate(glm::mat4(), -m_position);
		
		m_invViewMatrix = glm::translate(glm::mat4(), m_position) *
		                  m_rotationMatrix *
		                  glm::scale(glm::mat4(), glm::vec3(1, -1, 1));
		
		//forward = m_invViewMatrix * (0, 0, -1, 0)
		m_forward = -m_invViewMatrix[2];
	}
	
}
