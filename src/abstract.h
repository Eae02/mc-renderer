#pragma once

namespace MCR
{
	class Abstract
	{
	public:
		virtual ~Abstract() = default;
		
		Abstract(const Abstract& other) = default;
		Abstract& operator=(const Abstract& other) = default;
		
		Abstract(Abstract&& other) = default;
		Abstract& operator=(Abstract&& other) = default;
		
	protected:
		Abstract() = default;
	};
}
