#pragma once

#include <SDL2/SDL.h>
#include <vector>

#include "filesystem.h"

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
		
		void Load(const fs::path& path);
		void Save(const fs::path& path) const;
		
		inline void SetShadowQuality(QualityLevels quality)
		{
			m_shadowQuality = quality;
		}
		
		inline QualityLevels GetShadowQuality()
		{
			return m_shadowQuality;
		}
		
		inline bool IsFullScreen() const
		{
			return m_fullScreen;
		}
		
		inline int GetFullScreenDisplayModeIndex() const
		{
			return m_fullScreenDisplayMode;
		}
		
		inline int GetRenderDistance() const
		{
			return m_renderDistance;
		}
		
	private:
		static std::vector<SDL_DisplayMode> s_availableDisplayModes;
		static int s_defaultDisplayModeIndex;
		
		bool m_fullScreen;
		int m_fullScreenDisplayMode;
		
		int m_renderDistance;
		
		QualityLevels m_shadowQuality;
	};
}
