#pragma once

#include <SDL2/SDL.h>
#include <vector>

namespace MCR
{
	enum class QualityLevels
	{
		Low = 0,
		Medium = 1,
		High = 2,
		VeryHigh = 3
	};
	
	class Settings
	{
	public:
		Settings();
		
		static void QueryAvailableDisplayModes();
		
		inline void SetShadowQuality(QualityLevels quality)
		{
			m_shadowQuality = quality;
		}
		
		inline QualityLevels GetShadowQuality()
		{
			return m_shadowQuality;
		}
		
		inline bool IsFullscreen() const
		{
			return m_fullscreen;
		}
		
		inline int GetFullscreenDisplayModeIndex() const
		{
			return m_fullscreenDisplayMode;
		}
		
		inline int GetRenderDistance() const
		{
			return m_renderDistance;
		}
		
	private:
		static std::vector<SDL_DisplayMode> s_availableDisplayModes;
		static int s_defaultDisplayModeIndex;
		
		bool m_fullscreen;
		int m_fullscreenDisplayMode;
		
		int m_renderDistance;
		
		QualityLevels m_shadowQuality;
	};
}
