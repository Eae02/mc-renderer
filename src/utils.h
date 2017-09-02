#pragma once

#define MAKE_RANGE(x) std::begin(x), std::end(x)

#ifdef __cpp_if_constexpr
#define constexpr_if if constexpr
#else
#define constexpr_if if
#endif

#include <ostream>

#include <gsl/span>

namespace MCR
{
	template <typename T>
	inline gsl::span<T> SingleElementSpan(T& element)
	{
		return { &element, 1 };
	}
	
	std::ostream& GetLogStream();
}
