#pragma once

#include <string_view>
#include <algorithm>

#include "sides.h"
#include "../utils.h"

namespace MCR
{
	struct BlockTexConfig
	{
		std::string_view m_textures[6];
		
		inline static BlockTexConfig Uniform(std::string_view texture)
		{
			BlockTexConfig config;
			std::fill(MAKE_RANGE(config.m_textures), texture);
			return config;
		}
		
		inline static BlockTexConfig UniformSideTop(std::string_view topTexture, std::string_view sideTexture)
		{
			BlockTexConfig config;
			std::fill(MAKE_RANGE(config.m_textures), sideTexture);
			config.m_textures[BLOCK_SIDE_POSY] = topTexture;
			config.m_textures[BLOCK_SIDE_NEGY] = topTexture;
			return config;
		}
		
		inline static BlockTexConfig UniformSide(std::string_view topTexture, std::string_view sideTexture,
		                                         std::string_view bottomTexture)
		{
			BlockTexConfig config;
			std::fill(MAKE_RANGE(config.m_textures), sideTexture);
			config.m_textures[BLOCK_SIDE_POSY] = topTexture;
			config.m_textures[BLOCK_SIDE_NEGY] = bottomTexture;
			return config;
		}
	};
}
