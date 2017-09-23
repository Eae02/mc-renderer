#pragma once

#include <stdexcept>

namespace MCR
{
	class GraphicsDeviceLostException : public std::exception
	{
	public:
		const char* what() const noexcept override
		{
			return "Graphics device lost";
		}
	};
}
