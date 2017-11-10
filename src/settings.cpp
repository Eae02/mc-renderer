#include <SDL2/SDL_video.h>
#include "settings.h"

namespace MCR
{
	std::vector<SDL_DisplayMode> Settings::s_availableDisplayModes;
	int Settings::s_defaultDisplayModeIndex;
	
	inline bool DisplayModeEqual(const SDL_DisplayMode& a, const SDL_DisplayMode& b)
	{
		return a.w == b.w && a.h == b.h && a.refresh_rate == b.refresh_rate && a.format == b.format;
	}
	
	void Settings::QueryAvailableDisplayModes()
	{
		const int displayIndex = 0;
		
		SDL_DisplayMode desktopDisplayMode;
		SDL_GetDesktopDisplayMode(displayIndex, &desktopDisplayMode);
		
		const int numDisplayModes = SDL_GetNumDisplayModes(displayIndex);
		s_availableDisplayModes.resize(static_cast<size_t>(numDisplayModes));
		
		for (int i = 0; i < numDisplayModes; i++)
		{
			SDL_GetDisplayMode(displayIndex, i, &s_availableDisplayModes[i]);
			
			if (DisplayModeEqual(s_availableDisplayModes[i], desktopDisplayMode))
			{
				s_defaultDisplayModeIndex = i;
			}
		}
	}
	
	Settings::Settings()
		: m_fullScreen(false), m_fullScreenDisplayMode(s_defaultDisplayModeIndex),
		  m_renderDistance(6), m_shadowQuality(QualityLevels::Medium)
	{
		
	}
}
