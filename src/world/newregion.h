#pragma once

#include "region.h"

#include <memory>

namespace MCR
{
	struct NewRegion
	{
		std::shared_ptr<Region> m_region;
		
		inline explicit NewRegion(std::shared_ptr<Region> region = nullptr)
			: m_region(std::move(region)) { }
	};
}
