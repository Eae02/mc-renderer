#pragma once

#include <string_view>
#include <gsl/span>

namespace MCR
{
	struct BlockTextureDesc
	{
		std::string_view m_name;
		bool m_gray;
		
		inline BlockTextureDesc(std::string_view name, bool gray = false)
		    : m_name(name), m_gray(gray) { }
	};
	
	const gsl::span<const BlockTextureDesc> GetBlockTexturesList();
}
