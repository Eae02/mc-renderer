#pragma once

#include "region.h"

#include <memory>

namespace MCR
{
	struct NewRegion
	{
		std::shared_ptr<Region> m_region;
		bool m_hasWater;
		
		inline explicit NewRegion(std::shared_ptr<Region> region = nullptr, bool hasWater = false)
			: m_region(std::move(region)), m_hasWater(hasWater) { }
	};
}
