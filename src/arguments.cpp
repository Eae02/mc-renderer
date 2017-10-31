#include "arguments.h"

#include <cstring>

namespace MCR
{
namespace Arguments
{
	bool noValidation = false;
	bool noBackgroundTransfer = false;
	bool noVkExtensions = false;
	
	void Parse(int argc, char** argv)
	{
		for (int i = 1; i < argc; i++)
		{
			if (std::strcmp(argv[i], "--no-validation") == 0)
			{
				noValidation = true;
			}
			
			if (std::strcmp(argv[i], "--no-bkg-transfer") == 0)
			{
				noBackgroundTransfer = true;
			}
			
			if (std::strcmp(argv[i], "--no-vk-ext") == 0)
			{
				noVkExtensions = true;
			}
		}
	}
}
}
