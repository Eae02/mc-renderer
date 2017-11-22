#include <SDL2/SDL_video.h>
#include "settings.h"
#include "utils.h"

#include <cstdlib>
#include <json.hpp>
#include <fstream>

namespace MCR
{
	static const int displayIndex = 0;
	
	std::vector<SDL_DisplayMode> Settings::s_availableDisplayModes;
	int Settings::s_defaultDisplayModeIndex;
	
	inline bool DisplayModeEqual(const SDL_DisplayMode& a, const SDL_DisplayMode& b)
	{
		return a.w == b.w && a.h == b.h && a.refresh_rate == b.refresh_rate && a.format == b.format;
	}
	
	void Settings::QueryAvailableDisplayModes()
	{
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
		: m_enableVSync(true), m_fullScreen(false), m_fullScreenDisplayMode(s_defaultDisplayModeIndex),
		  m_renderDistance(10), m_shadowQuality(QualityLevels::Medium)
	{
		
	}
	
	static const std::string_view qualityLevelNames[] = { "Low", "Medium", "High", "VeryHigh" };
	
	static QualityLevels ParseQualityLevel(std::string_view qualityLevel)
	{
		for (int i = 0; i < static_cast<int>(ArrayLength(qualityLevelNames)); i++)
		{
			if (qualityLevelNames[i] == qualityLevel)
			{
				return static_cast<QualityLevels>(i);
			}
		}
		
		return QualityLevels::Medium;
	}
	
	static std::string QualityLevelToString(QualityLevels qualityLevel)
	{
		return std::string(qualityLevelNames[static_cast<int>(qualityLevel)]);
	}
	
	void Settings::Save(const fs::path& path) const
	{
		nlohmann::json json = 
		{
			{ "ResX", s_availableDisplayModes[m_fullScreenDisplayMode].w },
			{ "ResY", s_availableDisplayModes[m_fullScreenDisplayMode].h },
			{ "RefreshRate", s_availableDisplayModes[m_fullScreenDisplayMode].refresh_rate },
			{ "FullScreen", m_fullScreen },
			{ "VSync", m_enableVSync },
			{ "RenderDistance", m_renderDistance },
			{ "ShadowQuality", QualityLevelToString(m_shadowQuality) }
		};
		
		std::ofstream stream(path);
		stream << std::setw(4) << json;
	}
	
	void Settings::Load(const fs::path& path)
	{
		std::ifstream stream(path);
		nlohmann::json json = nlohmann::json::parse(stream);
		
		SDL_DisplayMode displayMode = s_availableDisplayModes[s_defaultDisplayModeIndex];
		
		auto resXIt = json.find("ResX");
		if (resXIt != json.end())
		{
			displayMode.w = *resXIt;
		}
		
		auto resYIt = json.find("ResY");
		if (resYIt != json.end())
		{
			displayMode.h = *resYIt;
		}
		
		auto refreshRateIt = json.find("RefreshRate");
		if (refreshRateIt != json.end())
		{
			displayMode.refresh_rate = *refreshRateIt;
		}
		
		//Finds the display mode in the list of available display modes.
		auto displayModeIt = std::find_if(MAKE_RANGE(s_availableDisplayModes), [&] (const SDL_DisplayMode& availDispMode)
		{
			return availDispMode.w == displayMode.w && availDispMode.h == displayMode.h &&
			       availDispMode.refresh_rate == displayMode.refresh_rate;
		});
		
		if (displayModeIt == s_availableDisplayModes.end())
		{
			m_fullScreenDisplayMode = s_defaultDisplayModeIndex;
		}
		else
		{
			m_fullScreenDisplayMode = static_cast<int>(displayModeIt - s_availableDisplayModes.begin());
		}
		
		auto fullScreenIt = json.find("FullScreen");
		if (fullScreenIt != json.end())
		{
			m_fullScreen = *fullScreenIt;
		}
		
		auto vSyncIt = json.find("VSync");
		if (vSyncIt != json.end())
		{
			m_enableVSync = *vSyncIt;
		}
		
		auto renderDistanceIt = json.find("RenderDistance");
		if (renderDistanceIt != json.end())
		{
			m_renderDistance = *renderDistanceIt;
		}
		
		auto shadowQualityIt = json.find("ShadowQuality");
		if (shadowQualityIt != json.end())
		{
			m_shadowQuality = ParseQualityLevel(shadowQualityIt->get<std::string>());
		}
	}
}
