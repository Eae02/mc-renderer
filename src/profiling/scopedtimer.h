#pragma once

#ifndef NO_PROF

#include <chrono>
#include <string_view>

#include "frameprofiler.h"

#define MCR_SCOPED_TIMER(id, desc) ScopedTimer _t ## id(desc)

namespace MCR
{
	class ScopedTimer
	{
	public:
		inline ScopedTimer(std::string_view name)
		    : m_id(BeginCPUTimer(name)) { }
		
		inline ~ScopedTimer()
		{
			EndCPUTimer(m_id);
		}
		
	private:
		uint32_t m_id;
	};
}

#else

#define MCR_SCOPED_TIMER(id, desc)

#endif
