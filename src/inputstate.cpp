#include "inputstate.h"
#include "utils.h"

namespace MCR
{
	InputState::InputState(const glm::vec2& cursorPos)
	{
		m_currentFrame.m_cursorPos = cursorPos;
		m_prevFrame.m_cursorPos = cursorPos;
		std::fill(MAKE_RANGE(m_currentFrame.m_keysPressed), false);
		std::fill(MAKE_RANGE(m_currentFrame.m_mouseButtonsPressed), false);
		std::fill(MAKE_RANGE(m_prevFrame.m_keysPressed), false);
		std::fill(MAKE_RANGE(m_prevFrame.m_mouseButtonsPressed), false);
	}
	
	void InputState::NewFrame()
	{
		m_prevFrame = m_currentFrame;
		m_cursorDelta = glm::vec2();
	}
	
	void InputState::MouseMotionEvent(const SDL_MouseMotionEvent& event)
	{
		m_currentFrame.m_cursorPos.x = event.x;
		m_currentFrame.m_cursorPos.y = event.y;
		
		m_cursorDelta.x += event.xrel;
		m_cursorDelta.y += event.yrel;
	}
	
	void InputState::MouseButtonEvent(uint8_t button, bool newState)
	{
		switch (button)
		{
		case SDL_BUTTON_LEFT:
			m_currentFrame.m_mouseButtonsPressed[static_cast<int>(MouseButtons::Left)] = newState;
			break;
		case SDL_BUTTON_RIGHT:
			m_currentFrame.m_mouseButtonsPressed[static_cast<int>(MouseButtons::Right)] = newState;
			break;
		case SDL_BUTTON_MIDDLE:
			m_currentFrame.m_mouseButtonsPressed[static_cast<int>(MouseButtons::Middle)] = newState;
			break;
		}
	}
	
	void InputState::KeyEvent(SDL_Scancode scancode, bool newState)
	{
		Keys key = TranslateKey(scancode);
		if (key != Keys::Unknown)
		{
			m_currentFrame.m_keysPressed[static_cast<int>(key)] = newState;
		}
	}
}
