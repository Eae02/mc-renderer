#pragma once

#include <glm/glm.hpp>
#include <array>
#include <SDL2/SDL.h>

#include "keys.h"

namespace MCR
{
	enum class MouseButtons
	{
		Left,
		Right,
		Middle
	};
	
	class InputState
	{
	public:
		explicit InputState(const glm::vec2& cursorPos);
		
		void NewFrame();
		
		void MouseMotionEvent(const SDL_MouseMotionEvent& event);
		
		void MouseButtonEvent(uint8_t button, bool newState);
		
		void KeyEvent(SDL_Scancode scancode, bool newState);
		
		inline bool IsButtonPressed(MouseButtons button) const
		{
			return m_currentFrame.m_mouseButtonsPressed[static_cast<int>(button)];
		}
		
		inline bool WasButtonPressed(MouseButtons button) const
		{
			return m_prevFrame.m_mouseButtonsPressed[static_cast<int>(button)];
		}
		
		inline bool IsKeyPressed(Keys key) const
		{
			return m_currentFrame.m_keysPressed[static_cast<int>(key)];
		}
		
		inline bool WasKeyPressed(Keys key) const
		{
			return m_prevFrame.m_keysPressed[static_cast<int>(key)];
		}
		
		inline const glm::vec2& GetCursorPos() const
		{
			return m_currentFrame.m_cursorPos;
		}
		
		inline const glm::vec2& GetPrevCursorPos() const
		{
			return m_prevFrame.m_cursorPos;
		}
		
		inline glm::vec2 GetCursorDelta() const
		{
			return m_currentFrame.m_cursorPos - m_prevFrame.m_cursorPos;
		}
		
	private:
		struct FrameState
		{
			glm::vec2 m_cursorPos;
			std::array<bool, 3> m_mouseButtonsPressed;
			std::array<bool, KeyCount> m_keysPressed;
		};
		
		FrameState m_currentFrame;
		FrameState m_prevFrame;
	};
}
